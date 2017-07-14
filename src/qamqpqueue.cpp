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
      consuming(false),
      consumeRequested(false),
      messageCount(0),
      consumerCount(0)
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


void QAmqpQueuePrivate::resetInternalState()
{
    QAmqpChannelPrivate::resetInternalState();
    delayedDeclare = false;
    declared = false;
    recievingMessage = false;
    consuming = false;
    consumeRequested = false;
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
    declared = true;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    name = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();

    stream >> messageCount >> consumerCount;

    qAmqpDebug("-> queue#declareOk( queue-name=%s, message-count=%d, consumer-count=%d )",
               qPrintable(name), messageCount, consumerCount);

    Q_EMIT q->declared();
}

void QAmqpQueuePrivate::purgeOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);


    stream >> messageCount;

    qAmqpDebug("-> queue#purgeOk( queue-name=%s, message-count=%d )",
               qPrintable(name), messageCount);

    Q_EMIT q->purged(messageCount);
}

void QAmqpQueuePrivate::deleteOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpQueue);
    declared = false;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    stream >> messageCount;

    qAmqpDebug("-> queue#deleteOk( queue-name=%s, message-count=%d )",
               qPrintable(name), messageCount);


    Q_EMIT q->removed();
}

void QAmqpQueuePrivate::bindOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAmqpQueue);
    qAmqpDebug("-> queue[ %s ]#bindOk()", qPrintable(name));
    Q_EMIT q->bound();
}

void QAmqpQueuePrivate::unbindOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAmqpQueue);
    qAmqpDebug("-> queue[ %s ]#unbindOk()", qPrintable(name));
    Q_EMIT q->unbound();
}

void QAmqpQueuePrivate::getOk(const QAmqpMethodFrame &frame)
{
    qAmqpDebug("-> queue[ %s ]#getOk()", qPrintable(name));

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
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    consumerTag = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    consuming = true;
    consumeRequested = false;

    qAmqpDebug("-> queue[ %s ]#consumeOk( consumer-tag=%s )", qPrintable(name), qPrintable(consumerTag));

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

    QByteArray args;
    QDataStream out(&args, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, name);
    out << qint8(options);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, arguments);

    qAmqpDebug("<- queue#declare( queue=%s, passive=%d, durable=%d, exclusive=%d, auto-delete=%d, no-wait=%d )",
               qPrintable(name), options & QAmqpQueue::Passive, options & QAmqpQueue::Durable,
               options & QAmqpQueue::Exclusive, options & QAmqpQueue::AutoDelete,
               options & QAmqpQueue::NoWait);

    frame.setArguments(args);
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

    qAmqpDebug("-> queue[ %s ]#cancelOk( consumer-tag=%s )", qPrintable(name), qPrintable(consumerTag));

    consumerTag.clear();
    consuming = false;
    consumeRequested = false;
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

qint32 QAmqpQueue::messageCount() const
{
    Q_D(const QAmqpQueue);
    return d->messageCount;
}

qint32 QAmqpQueue::consumerCount() const
{
    Q_D(const QAmqpQueue);
    return d->consumerCount;
}

void QAmqpQueue::declare(int options, const QAmqpTable &arguments)
{
    Q_D(QAmqpQueue);
    d->options = options;
    d->arguments = arguments;

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

    qAmqpDebug("<- queue#delete( queue=%s, if-unused=%d, if-empty=%d )",
               qPrintable(d->name), options & QAmqpQueue::roIfUnused, options & QAmqpQueue::roIfEmpty);

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

    qAmqpDebug("<- queue#purge( queue=%s, no-wait=%d )", qPrintable(d->name), 0);

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

    qAmqpDebug("<- queue#bind( queue=%s, exchange=%s, routing-key=%s, no-wait=%d )",
               qPrintable(d->name), qPrintable(exchangeName), qPrintable(key),
               0);

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

    qAmqpDebug("<- queue#unbind( queue=%s, exchange=%s, routing-key=%s )",
               qPrintable(d->name), qPrintable(exchangeName), qPrintable(key));

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

    if (d->consumeRequested) {
        qAmqpDebug() << Q_FUNC_INFO << "already attempting to consume";
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

    qAmqpDebug("<- basic#consume( queue=%s, consumer-tag=%s, no-local=%d, no-ack=%d, exclusive=%d, no-wait=%d )",
               qPrintable(d->name), qPrintable(d->consumerTag),
               options & QAmqpQueue::coNoLocal, options & QAmqpQueue::coNoAck,
               options & QAmqpQueue::coExclusive, options & QAmqpQueue::coNoWait);

    frame.setArguments(arguments);
    d->sendFrame(frame);
    d->consumeRequested = true;
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

    qAmqpDebug("<- basic#get( queue=%s, no-ack=%d )", qPrintable(d->name), noAck);

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

    qAmqpDebug("<- basic#ack( delivery-tag=%llu, multiple=%d )", deliveryTag, multiple);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpQueue::reject(const QAmqpMessage &message, bool requeue)
{
    reject(message.deliveryTag(), requeue);
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

    qAmqpDebug("<- basic#reject( delivery-tag=%llu, requeue=%d )", deliveryTag, requeue);

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

    qAmqpDebug("<- basic#cancel( consumer-tag=%s, no-wait=%d )", qPrintable(d->consumerTag), noWait);

    frame.setArguments(arguments);
    d->sendFrame(frame);
    return true;
}
