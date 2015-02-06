#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QFile>

#include "qamqpclient.h"
#include "qamqpclient_p.h"
#include "qamqpqueue.h"
#include "qamqpqueue_p.h"
#include "qamqpexchange.h"
#include "qamqpmessage_p.h"
#include "qamqptable.h"
using namespace QAMQP;

QAmqpQueuePrivate::QAmqpQueuePrivate(QAmqpQueue *q)
    : QAmqpChannelPrivate(q),
      delayedDeclare(false),
      declared(false),
      recievingMessage(false),
      consuming(false)
{
}

QAmqpQueuePrivate::~QAmqpQueuePrivate()
{
    if (!client.isNull()) {
        QAmqpClientPrivate *priv = client->d_func();
        priv->contentHandlerByChannel[channelNumber].removeAll(this);
        priv->bodyHandlersByChannel[channelNumber].removeAll(this);
    }
}

bool QAmqpQueuePrivate::_q_method(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    if (QAmqpChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() == QAmqpFrame::Queue) {
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

    if (frame.methodClass() == QAmqpFrame::Basic) {
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

void QAmqpQueuePrivate::_q_content(const QAmqpContentFrame &frame)
{
    Q_Q(QAmqpQueue);
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return;

    if (!currentMessage.isValid()) {
        qAmqpDebug() << "received content-header without delivered message";
        return;
    }

    currentMessage.d->leftSize = frame.bodySize();
    QAmqpMessage::PropertyHash::ConstIterator it;
    QAmqpMessage::PropertyHash::ConstIterator itEnd = frame.properties_.constEnd();
    for (it = frame.properties_.constBegin(); it != itEnd; ++it) {
        QAmqpMessage::Property property = (it.key());
        if (property == QAmqpMessage::Headers)
            currentMessage.d->headers = (it.value()).toHash();
        currentMessage.d->properties[property] = it.value();
    }

    if (currentMessage.d->leftSize == 0) {
        // message with an empty body
        q->enqueue(currentMessage);
        Q_EMIT q->messageReceived();
    }
}

void QAmqpQueuePrivate::_q_body(const QAmqpContentBodyFrame &frame)
{
    Q_Q(QAmqpQueue);
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

void QAmqpQueuePrivate::declareOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    qAmqpDebug() << "declared queue: " << name;
    declared = true;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    name = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    qint32 messageCount = 0, consumerCount = 0;
    stream >> messageCount >> consumerCount;
    qAmqpDebug("message count %d\nConsumer count: %d", messageCount, consumerCount);

    Q_EMIT q->declared();
}

void QAmqpQueuePrivate::purgeOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    qAmqpDebug() << "purged queue: " << name;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    qint32 messageCount = 0;
    stream >> messageCount;

    Q_EMIT q->purged(messageCount);
}

void QAmqpQueuePrivate::deleteOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    qAmqpDebug() << "deleted queue: " << name;
    declared = false;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint32 messageCount = 0;
    stream >> messageCount;
    qAmqpDebug("Message count %d", messageCount);

    Q_EMIT q->removed();
}

void QAmqpQueuePrivate::bindOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)

    Q_Q(QAmqpQueue);
    qAmqpDebug() << Q_FUNC_INFO << "bound to exchange";
    Q_EMIT q->bound();
}

void QAmqpQueuePrivate::unbindOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)

    Q_Q(QAmqpQueue);
    qAmqpDebug() << Q_FUNC_INFO << "unbound from exchange";
    Q_EMIT q->unbound();
}

void QAmqpQueuePrivate::getOk(const QAmqpMethodFrame &frame)
{
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);

    QAmqpMessage message;
    message.d->deliveryTag = QAmqpFrame::readAmqpField(in, QAmqpMetaType::LongLongUint).toLongLong();
    message.d->redelivered = QAmqpFrame::readAmqpField(in, QAmqpMetaType::Boolean).toBool();
    message.d->exchangeName = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    message.d->routingKey = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    currentMessage = message;
}

void QAmqpQueuePrivate::consumeOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    qAmqpDebug() << "consume ok: " << name;
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    consumerTag = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    qAmqpDebug("consumer tag = %s", qPrintable(consumerTag));
    consuming = true;
    Q_EMIT q->consuming(consumerTag);
}

void QAmqpQueuePrivate::deliver(const QAmqpMethodFrame &frame)
{
    qAmqpDebug() << Q_FUNC_INFO;
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);
    QString consumer = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    if (consumerTag != consumer) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid consumer tag: " << consumer;
        return;
    }

    QAmqpMessage message;
    message.d->deliveryTag = QAmqpFrame::readAmqpField(in, QAmqpMetaType::LongLongUint).toLongLong();
    message.d->redelivered = QAmqpFrame::readAmqpField(in, QAmqpMetaType::Boolean).toBool();
    message.d->exchangeName = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    message.d->routingKey = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    currentMessage = message;
}

void QAmqpQueuePrivate::declare()
{
    QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miDeclare);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, name);
    out << qint8(options);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

    frame.setArguments(arguments);
    sendFrame(frame);

    if (delayedDeclare)
        delayedDeclare = false;
}

void QAmqpQueuePrivate::cancelOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    qAmqpDebug() << Q_FUNC_INFO;
    QByteArray data = frame.arguments();
    QDataStream in(&data, QIODevice::ReadOnly);
    QString consumer = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString).toString();
    if (consumerTag != consumer) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid consumer tag: " << consumer;
        return;
    }

    consumerTag.clear();
    Q_EMIT q->cancelled(consumer);
}

//////////////////////////////////////////////////////////////////////////

QAmqpQueue::QAmqpQueue(int channelNumber, QAmqpClient *parent)
    : QAmqpChannel(new QAmqpQueuePrivate(this), parent)
{
    Q_D(QAmqpQueue);
    d->init(channelNumber, parent);
}

QAmqpQueue::~QAmqpQueue()
{
}

void QAmqpQueue::channelOpened()
{
    Q_D(QAmqpQueue);
    if (d->delayedDeclare)
        d->declare();

    if (!d->delayedBindings.isEmpty()) {
        typedef QPair<QString, QString> BindingPair;
        foreach(BindingPair binding, d->delayedBindings)
            bind(binding.first, binding.second);
        d->delayedBindings.clear();
    }
}

void QAmqpQueue::channelClosed()
{
}

int QAmqpQueue::options() const
{
    Q_D(const QAmqpQueue);
    return d->options;
}

void QAmqpQueue::declare(int options)
{
    Q_D(QAmqpQueue);
    d->options = options;

    if (!d->opened) {
        d->delayedDeclare = true;
        return;
    }

    d->declare();
}

void QAmqpQueue::remove(int options)
{
    Q_D(QAmqpQueue);
    if (!d->declared) {
        qAmqpDebug() << Q_FUNC_INFO << "trying to remove undeclared queue, aborting...";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miDelete);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    out << qint8(options);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::purge()
{
    Q_D(QAmqpQueue);

    if (!d->opened)
        return;

    QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miPurge);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);
    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    out << qint8(0);    // no-wait

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::bind(QAmqpExchange *exchange, const QString &key)
{
    if (!exchange) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid exchange provided";
        return;
    }

    bind(exchange->name(), key);
}

void QAmqpQueue::bind(const QString &exchangeName, const QString &key)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        d->delayedBindings.append(QPair<QString,QString>(exchangeName, key));
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miBind);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //  reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, exchangeName);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, key);

    out << qint8(0);    //  no-wait
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::unbind(QAmqpExchange *exchange, const QString &key)
{
    if (!exchange) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid exchange provided";
        return;
    }

    unbind(exchange->name(), key);
}

void QAmqpQueue::unbind(const QString &exchangeName, const QString &key)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miUnbind);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);
    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, exchangeName);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, key);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

bool QAmqpQueue::consume(int options)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not open";
        return false;
    }

    if (d->consuming) {
        qAmqpDebug() << Q_FUNC_INFO << "already consuming with tag: " << d->consumerTag;
        return false;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpQueuePrivate::bmConsume);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->consumerTag);

    out << qint8(options);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

    frame.setArguments(arguments);
    d->sendFrame(frame);
    return true;
}

void QAmqpQueue::setConsumerTag(const QString &consumerTag)
{
    Q_D(QAmqpQueue);
    d->consumerTag = consumerTag;
}

QString QAmqpQueue::consumerTag() const
{
    Q_D(const QAmqpQueue);
    return d->consumerTag;
}

bool QAmqpQueue::isConsuming() const
{
    Q_D(const QAmqpQueue);
    return d->consuming;
}

bool QAmqpQueue::isDeclared() const
{
    Q_D(const QAmqpQueue);
    return d->declared;
}

void QAmqpQueue::get(bool noAck)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "channel is not open";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpQueuePrivate::bmGet);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    out << qint8(noAck ? 1 : 0); // no-ack

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::ack(const QAmqpMessage &message)
{
    ack(message.deliveryTag(), false);
}

void QAmqpQueue::ack(qlonglong deliveryTag, bool multiple)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "channel is not open";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpQueuePrivate::bmAck);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << deliveryTag;
    out << qint8(multiple ? 1 : 0); // multiple

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::reject(const QAmqpMessage &message, bool requeue)
{
    ack(message.deliveryTag(), requeue);
}

void QAmqpQueue::reject(qlonglong deliveryTag, bool requeue)
{
    Q_D(QAmqpQueue);
    if (!d->opened) {
        qAmqpDebug() << Q_FUNC_INFO << "channel is not open";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpQueuePrivate::bmReject);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << deliveryTag;
    out << qint8(requeue ? 1 : 0);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

bool QAmqpQueue::cancel(bool noWait)
{
    Q_D(QAmqpQueue);
    if (!d->consuming) {
        qAmqpDebug() << Q_FUNC_INFO << "not consuming!";
        return false;
    }

    if (d->consumerTag.isEmpty()) {
        qAmqpDebug() << Q_FUNC_INFO << "consuming with an empty consumer tag, failing...";
        return false;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpQueuePrivate::bmCancel);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->consumerTag);
    out << (noWait ? qint8(0x01) : qint8(0x0));

    frame.setArguments(arguments);
    d->sendFrame(frame);
    return true;
}

#include "moc_qamqpqueue.cpp"
