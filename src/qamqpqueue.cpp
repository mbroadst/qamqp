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
      queueState(Q_CLOSED),
      receivingMessage(false),
      consumeOptions(0),
      consumerState(C_UNDECLARED),
      delayedConsume(false)
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
    queueState = Q_DECLARED;
    consumerState = C_DECLARED;

    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    name = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    qint32 messageCount = 0, consumerCount = 0;
    stream >> messageCount >> consumerCount;
    qAmqpDebug("message count %d\nConsumer count: %d", messageCount, consumerCount);

    if (delayedConsume)
        q->consume(consumeOptions);
    else
        processBindings();

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
    queueState = Q_UNDECLARED;
    consumerState = C_UNDECLARED;

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
    qAmqpDebug() << Q_FUNC_INFO << "bound to exchange"
        << bindPendingExchange << "key" << bindPendingKey;
    Q_EMIT q->bound();

    QString exchangeName(bindPendingExchange);
    QString key(bindPendingKey);

    bindPendingExchange.clear();
    bindPendingKey.clear();

    if (exchangeName.isEmpty() || key.isEmpty())
        return;

    SubscriptionState& state(getState(exchangeName));
    state.topicsToBind.remove(key);
    state.topics << key;
    processBindings();
}

void QAmqpQueuePrivate::unbindOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)

    Q_Q(QAmqpQueue);
    qAmqpDebug() << Q_FUNC_INFO << "unbound from exchange"
        << bindPendingExchange << "key" << bindPendingKey;
    Q_EMIT q->unbound();

    QString exchangeName(bindPendingExchange);
    QString key(bindPendingKey);

    bindPendingExchange.clear();
    bindPendingKey.clear();

    if (exchangeName.isEmpty() || key.isEmpty())
        return;

    SubscriptionState& state(getState(exchangeName));
    state.topicsToUnbind.remove(key);
    state.topics.remove(key);
    processBindings();

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
    consumerState = C_CONSUMING;
    delayedConsume = false;
    processBindings();
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
    consumerState = C_DECLARED;
    delayedConsume = false;
    Q_EMIT q->cancelled(consumer);
}

/*!
 * Retrieve the state of an exchange's bindings.
 */
QAmqpQueuePrivate::SubscriptionState&
QAmqpQueuePrivate::getState(const QString& exchangeName)
{
    SubscriptionState& state = bindings[exchangeName];
    if (state.exchange == NULL)
        state.exchange = client->createExchange(exchangeName);
    return state;
}

/*!
 * Reset the binding state.  This marks all presently "subscribed" exchanges
 * and keys as "to be subscribed" except those presently marked as "to be
 * unsubscribed".
 */
void QAmqpQueuePrivate::resetBindings()
{
    /* Reset the current binding operation */
    bindPendingExchange.clear();
    bindPendingKey.clear();

    /* Get a list of all bindings */
    QHash<QString, SubscriptionState> bindings(this->bindings);
    QHash<QString, SubscriptionState>::iterator bIt;

    for (bIt = bindings.begin(); bIt != bindings.end(); bIt++) {
        SubscriptionState&  state(bIt.value());

        /* Remove all contents of the "to unbind" from "currently bound" */
        state.topics.subtract(state.topicsToUnbind);

        /* Add the contents of "currently bound" to "to bind" */
        state.topicsToBind.unite(state.topics);

        /*
         * Clear out the currently bound topics.  We leave the contents of
         * toUnbind alone, since the queue may still exist and still have those
         * exchanges/keys bound to it.
         */
        state.topics.clear();
    }
    this->bindings = bindings;
}

/*!
 * Process binding list.
 */
void QAmqpQueuePrivate::processBindings()
{
    /* Do not proceed if a bind or unbind is pending. */
    if (!bindPendingExchange.isEmpty())
        return;

    /* Get a list of all bindings */
    QHash<QString, SubscriptionState> bindings(this->bindings);
    QHash<QString, SubscriptionState>::iterator bIt;
    QSet<QString>::iterator tIt;

    for (bIt = bindings.begin(); bIt != bindings.end(); bIt++) {
        QString             exchangeName(bIt.key());
        SubscriptionState&  state(bIt.value());

        /* Check for bindings to create */
        for (tIt = state.topicsToBind.begin();
                tIt != state.topicsToBind.end();
                tIt++) {
            QString         key(*tIt);
            bindPendingExchange = exchangeName;
            bindPendingKey = key;

            QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miBind);
            frame.setChannel(channelNumber);

            QByteArray arguments;
            QDataStream out(&arguments, QIODevice::WriteOnly);

            out << qint16(0);   //  reserved 1
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, name);
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, exchangeName);
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, key);

            out << qint8(0);    //  no-wait
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

            frame.setArguments(arguments);
            sendFrame(frame);
            /* Stop processing the list here */
            return;
        }

        /* Check for bindings to destroy */
        for (tIt = state.topicsToUnbind.begin();
                tIt != state.topicsToUnbind.end();
                tIt++) {
            QString         key(*tIt);
            bindPendingExchange = exchangeName;
            bindPendingKey = key;

            QAmqpMethodFrame frame(QAmqpFrame::Queue, QAmqpQueuePrivate::miUnbind);
            frame.setChannel(channelNumber);

            QByteArray arguments;
            QDataStream out(&arguments, QIODevice::WriteOnly);
            out << qint16(0);   //reserved 1
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, name);
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, exchangeName);
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, key);
            QAmqpFrame::writeAmqpField(out, QAmqpMetaType::Hash, QAmqpTable());

            frame.setArguments(arguments);
            sendFrame(frame);
            return;
        }
    }
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
    d->resetBindings();
    if (d->delayedDeclare)
        d->declare();
}

void QAmqpQueue::channelClosed()
{
    Q_D(QAmqpQueue);
    if (d->consumerState == QAmqpQueuePrivate::C_CONSUMING)
        d->delayedConsume = true;
    d->resetBindings();
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

    if (d->channelState != QAmqpChannelPrivate::CH_OPEN) {
        d->delayedDeclare = true;
        return;
    }

    d->declare();
}

void QAmqpQueue::remove(int options)
{
    Q_D(QAmqpQueue);
    if (d->queueState != QAmqpQueuePrivate::Q_DECLARED) {
        qAmqpDebug() << Q_FUNC_INFO << "trying to remove undeclared queue, aborting...";
        return;
    }

    d->queueState = QAmqpQueuePrivate::Q_REMOVING;
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

    if (d->channelState != QAmqpChannelPrivate::CH_OPEN)
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
    QAmqpQueuePrivate::SubscriptionState& subState = d->getState(exchangeName);
    subState.topicsToUnbind.remove(key);
    if (subState.topics.contains(key)) {
        qAmqpDebug() << Q_FUNC_INFO << "Already bound to exchange"
                     << exchangeName << "topic" << key;
        return;
    }

    subState.topicsToBind << key;
    if (d->channelState != QAmqpChannelPrivate::CH_OPEN)
        return;

    d->processBindings();
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
    if (!d->bindings.contains(exchangeName)) {
        qAmqpDebug() << Q_FUNC_INFO << "queue is not bound to "
                                    << exchangeName;
        return;
    }

    QAmqpQueuePrivate::SubscriptionState& subState = d->getState(exchangeName);
    subState.topicsToBind.remove(key);
    if (!subState.topics.contains(key)) {
        qAmqpDebug() << Q_FUNC_INFO << "Not bound to exchange"
                     << exchangeName << "topic" << key;
        return;
    }

    subState.topicsToUnbind << key;
    if (d->channelState != QAmqpChannelPrivate::CH_OPEN)
        return;

    d->processBindings();
}

bool QAmqpQueue::consume(int options)
{
    Q_D(QAmqpQueue);
    if (d->queueState != QAmqpQueuePrivate::Q_DECLARED) {
        d->consumeOptions = options;
        d->delayedConsume = true;
        return true;
    }

    switch(d->consumerState) {
        case QAmqpQueuePrivate::C_UNDECLARED:
            d->consumeOptions = options;
            d->delayedConsume = true;
            return true;
        case QAmqpQueuePrivate::C_REQUESTED:
            qAmqpDebug() << Q_FUNC_INFO << "already attempting to consume";
            return false;
        case QAmqpQueuePrivate::C_CONSUMING:
            qAmqpDebug() << Q_FUNC_INFO << "already consuming with tag: " << d->consumerTag;
            return false;
        case QAmqpQueuePrivate::C_CANCELLING:
            qAmqpDebug() << Q_FUNC_INFO << "attempting to cancel";
            return false;
        default:
            break;
    }

    d->consumerState = QAmqpQueuePrivate::C_REQUESTED;
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
    return d->consumerState == QAmqpQueuePrivate::C_CONSUMING;
}

bool QAmqpQueue::isDeclared() const
{
    Q_D(const QAmqpQueue);
    return d->queueState == QAmqpQueuePrivate::Q_DECLARED;
}

void QAmqpQueue::get(bool noAck)
{
    Q_D(QAmqpQueue);
    if (d->queueState != QAmqpQueuePrivate::Q_DECLARED) {
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
    if (d->queueState != QAmqpQueuePrivate::Q_DECLARED) {
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
    if (d->queueState != QAmqpQueuePrivate::Q_DECLARED) {
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

    if (d->consumerState != QAmqpQueuePrivate::C_CONSUMING) {
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
