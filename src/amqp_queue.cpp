#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_exchange.h"
#include "amqp_message_p.h"

using namespace QAMQP;

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QFile>

QueuePrivate::QueuePrivate(Queue *q)
    : ChannelPrivate(q),
      delayedDeclare(false),
      declared(false),
      noAck(true),
      recievingMessage(false),
      consuming(false)
{
}

QueuePrivate::~QueuePrivate()
{
}

bool QueuePrivate::_q_method(const Frame::Method &frame)
{
    Q_Q(Queue);
    if (ChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() == Frame::fcQueue) {
        switch (frame.id()) {
        case miDeclareOk:
            declareOk(frame);
            break;
        case miDeleteOk:
            deleteOk(frame);
            break;
        case miBindOk:
            bindOk(frame);
            break;
        case miUnbindOk:
            unbindOk(frame);
            break;
        case miPurgeOk:
            purgeOk(frame);
            break;
        }

        return true;
    }

    if (frame.methodClass() == Frame::fcBasic) {
        switch(frame.id()) {
        case bmConsumeOk:
            consumeOk(frame);
            break;
        case bmDeliver:
            deliver(frame);
            break;
        case bmGetOk:
            getOk(frame);
            break;
        case bmGetEmpty:
            Q_EMIT q->empty();
            break;
        case bmCancelOk:
            cancelOk(frame);
            break;
        }

        return true;
    }

    return false;
}

void QueuePrivate::_q_content(const Frame::Content &frame)
{
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return;

    if (!currentMessage.isValid()) {
        qAmqpDebug() << "received content-header without delivered message";
        return;
    }

    currentMessage.d->leftSize = frame.bodySize();
    QHash<int, QVariant>::ConstIterator it;
    QHash<int, QVariant>::ConstIterator itEnd = frame.properties_.constEnd();
    for (it = frame.properties_.constBegin(); it != itEnd; ++it)
        currentMessage.d->properties[MessageProperty(it.key())] = it.value();
}

void QueuePrivate::_q_body(const Frame::ContentBody &frame)
{
    Q_Q(Queue);
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return;

    if (!currentMessage.isValid()) {
        qAmqpDebug() << "received content-body without delivered message";
        return;
    }

    currentMessage.d->payload.append(frame.body());
    currentMessage.d->leftSize -= frame.body().size();
    if (currentMessage.d->leftSize == 0) {
        q->enqueue(currentMessage);
        Q_EMIT q->messageReceived();
    }
}

void QueuePrivate::declareOk(const Frame::Method &frame)
{
    Q_Q(Queue);
    qAmqpDebug() << "declared queue: " << name;
    declared = true;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    name = Frame::readField('s', stream).toString();
    qint32 messageCount = 0, consumerCount = 0;
    stream >> messageCount >> consumerCount;
    qAmqpDebug("message count %d\nConsumer count: %d", messageCount, consumerCount);

    Q_EMIT q->declared();
}

void QueuePrivate::purgeOk(const Frame::Method &frame)
{
    Q_Q(Queue);
    qAmqpDebug() << "purged queue: " << name;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    qint32 messageCount = 0;
    stream >> messageCount;

    Q_EMIT q->purged(messageCount);
}

void QueuePrivate::deleteOk(const Frame::Method &frame)
{
    Q_Q(Queue);
    qAmqpDebug() << "deleted queue: " << name;
    declared = false;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint32 messageCount = 0;
    stream >> messageCount;
    qAmqpDebug("Message count %d", messageCount);

    Q_EMIT q->removed();
}

void QueuePrivate::bindOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Queue);
    qAmqpDebug() << Q_FUNC_INFO << "bound to exchange";
    Q_EMIT q->bound();
}

void QueuePrivate::unbindOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Queue);
    qAmqpDebug() << Q_FUNC_INFO << "unbound from exchange";
    Q_EMIT q->unbound();
}

void QueuePrivate::getOk(const Frame::Method &frame)
{
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);

    Message message;
    message.d->deliveryTag = Frame::readField('L',in).toLongLong();
    message.d->redelivered = Frame::readField('t',in).toBool();
    message.d->exchangeName = Frame::readField('s',in).toString();
    message.d->routingKey = Frame::readField('s',in).toString();
    currentMessage = message;
}

void QueuePrivate::consumeOk(const Frame::Method &frame)
{
    Q_Q(Queue);
    qAmqpDebug() << "consume ok: " << name;
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    consumerTag = Frame::readField('s',stream).toString();
    qAmqpDebug("consumer tag = %s", qPrintable(consumerTag));
    consuming = true;
    Q_EMIT q->consuming(consumerTag);
}

void QueuePrivate::deliver(const Frame::Method &frame)
{
    qAmqpDebug() << Q_FUNC_INFO;
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);
    QString consumer = Frame::readField('s',in).toString();
    if (consumerTag != consumer) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid consumer tag: " << consumer;
        return;
    }

    Message message;
    message.d->deliveryTag = Frame::readField('L',in).toLongLong();
    message.d->redelivered = Frame::readField('t',in).toBool();
    message.d->exchangeName = Frame::readField('s',in).toString();
    message.d->routingKey = Frame::readField('s',in).toString();
    currentMessage = message;
}

void QueuePrivate::declare()
{
    Frame::Method frame(Frame::fcQueue, QueuePrivate::miDeclare);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, name);
    out << qint8(options);
    Frame::writeField('F', out, Frame::TableField());

    frame.setArguments(arguments);
    sendFrame(frame);

    if (delayedDeclare)
        delayedDeclare = false;
}

void QueuePrivate::cancelOk(const Frame::Method &frame)
{
    Q_Q(Queue);
    qAmqpDebug() << Q_FUNC_INFO;
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);
    QString consumer = Frame::readField('s',in).toString();
    if (consumerTag != consumer) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid consumer tag: " << consumer;
        return;
    }

    consumerTag.clear();
    Q_EMIT q->cancelled(consumer);
}

//////////////////////////////////////////////////////////////////////////

Queue::Queue(int channelNumber, Client *parent)
    : Channel(new QueuePrivate(this), parent)
{
    Q_D(Queue);
    d->init(channelNumber, parent);
}

Queue::~Queue()
{
}

void Queue::channelOpened()
{
    Q_D(Queue);
    if (d->delayedDeclare)
        d->declare();

    if (!d->delayedBindings.isEmpty()) {
        typedef QPair<QString, QString> BindingPair;
        foreach(BindingPair binding, d->delayedBindings)
            bind(binding.first, binding.second);
        d->delayedBindings.clear();
    }
}

void Queue::channelClosed()
{
}

int Queue::options() const
{
    Q_D(const Queue);
    return d->options;
}

void Queue::setNoAck(bool noAck)
{
    Q_D(Queue);
    d->noAck = noAck;
}

bool Queue::noAck() const
{
    Q_D(const Queue);
    return d->noAck;
}

void Queue::declare(int options)
{
    Q_D(Queue);
    d->options = options;

    if (!d->opened) {
        d->delayedDeclare = true;
        return;
    }

    d->declare();
}

void Queue::remove(int options)
{
    Q_D(Queue);
    if (!d->declared) {
        qAmqpDebug() << Q_FUNC_INFO << "trying to remove undeclared queue, aborting...";
        return;
    }

    Frame::Method frame(Frame::fcQueue, QueuePrivate::miDelete);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);
    out << qint8(options);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Queue::purge()
{
    Q_D(Queue);

    if (!d->opened)
        return;

    Frame::Method frame(Frame::fcQueue, QueuePrivate::miPurge);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);
    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);
    out << qint8(0);    // no-wait

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Queue::bind(Exchange *exchange, const QString &key)
{
    if (!exchange) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid exchange provided";
        return;
    }

    bind(exchange->name(), key);
}

void Queue::bind(const QString &exchangeName, const QString &key)
{
    Q_D(Queue);
    if (!d->opened) {
        d->delayedBindings.append(QPair<QString,QString>(exchangeName, key));
        return;
    }

    Frame::Method frame(Frame::fcQueue, QueuePrivate::miBind);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //  reserved 1
    Frame::writeField('s', out, d->name);
    Frame::writeField('s', out, exchangeName);
    Frame::writeField('s', out, key);

    out << qint8(0);    //  no-wait
    Frame::writeField('F', out, Frame::TableField());

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Queue::unbind(Exchange *exchange, const QString &key)
{
    if (!exchange) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid exchange provided";
        return;
    }

    unbind(exchange->name(), key);
}

void Queue::unbind(const QString &exchangeName, const QString &key)
{
    Q_D(Queue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return;
    }

    Frame::Method frame(Frame::fcQueue, QueuePrivate::miUnbind);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);
    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);
    Frame::writeField('s', out, exchangeName);
    Frame::writeField('s', out, key);
    Frame::writeField('F', out, Frame::TableField());

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

bool Queue::consume(int options)
{
    Q_D(Queue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return false;
    }

    if (d->consuming) {
        qAmqpDebug() << Q_FUNC_INFO << "already consuming with tag: " << d->consumerTag;
        return false;
    }

    Frame::Method frame(Frame::fcBasic, QueuePrivate::bmConsume);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);
    Frame::writeField('s', out, d->consumerTag);

    out << qint8(options);
    Frame::writeField('F', out, Frame::TableField());

    frame.setArguments(arguments);
    d->sendFrame(frame);
    return true;
}

void Queue::setConsumerTag(const QString &consumerTag)
{
    Q_D(Queue);
    d->consumerTag = consumerTag;
}

QString Queue::consumerTag() const
{
    Q_D(const Queue);
    return d->consumerTag;
}

bool Queue::isConsuming() const
{
    Q_D(const Queue);
    return d->consuming;
}

void Queue::get()
{
    Q_D(Queue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return;
    }

    Frame::Method frame(Frame::fcBasic, QueuePrivate::bmGet);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);

    out << qint8(d->noAck ? 1 : 0); // noAck

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Queue::ack(const Message &message)
{
    Q_D(Queue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return;
    }

    Frame::Method frame(Frame::fcBasic, QueuePrivate::bmAck);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << message.deliveryTag();   // reserved 1
    out << qint8(0);                // noAck

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

bool Queue::cancel(bool noWait)
{
    Q_D(Queue);
    if (!d->consuming) {
        qAmqpDebug() << Q_FUNC_INFO << "not consuming!";
        return false;
    }

    if (d->consumerTag.isEmpty()) {
        qAmqpDebug() << Q_FUNC_INFO << "consuming with an empty consumer tag, failing...";
        return false;
    }

    Frame::Method frame(Frame::fcBasic, QueuePrivate::bmCancel);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s', out, d->consumerTag);
    out << (noWait ? qint8(0x01) : qint8(0x0));

    frame.setArguments(arguments);
    d->sendFrame(frame);
    return true;
}
