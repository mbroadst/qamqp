#include <QEventLoop>
#include <QDataStream>
#include <QTimer>
#include <QDebug>

#include "qamqpexchange.h"
#include "qamqpexchange_p.h"
#include "qamqpqueue.h"
#include "qamqpglobal.h"
#include "qamqpclient.h"

QString QAmqpExchangePrivate::typeToString(QAmqpExchange::ExchangeType type)
{
    switch (type) {
    case QAmqpExchange::Direct: return QLatin1String("direct");
    case QAmqpExchange::FanOut: return QLatin1String("fanout");
    case QAmqpExchange::Topic: return QLatin1String("topic");
    case QAmqpExchange::Headers: return QLatin1String("headers");
    }

    return QLatin1String("direct");
}

QAmqpExchangePrivate::QAmqpExchangePrivate(QAmqpExchange *q)
    : QAmqpChannelPrivate(q),
      delayedDeclare(false),
      declared(false),
      nextDeliveryTag(0),
      pendingPublishBytes(0)
{
}

void QAmqpExchangePrivate::resetInternalState()
{
    QAmqpChannelPrivate::resetInternalState();
    delayedDeclare = false;
    declared = false;
    nextDeliveryTag = 0;
}

void QAmqpExchangePrivate::declare()
{
    if (!opened) {
        delayedDeclare = true;
        return;
    }

    if (name.isEmpty()) {
        qAmqpDebug() << Q_FUNC_INFO << "attempting to declare an unnamed exchange, aborting...";
        return;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Exchange, QAmqpExchangePrivate::miDeclare);
    frame.setChannel(channelNumber);

    QByteArray args;
    QDataStream stream(&args, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, name);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, type);

    stream << qint8(options);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::Hash, arguments);

    qAmqpDebug("<- exchange#declare( name=%s, type=%s, passive=%d, durable=%d, no-wait=%d )",
               qPrintable(name), qPrintable(type),
               options.testFlag(QAmqpExchange::Passive), options.testFlag(QAmqpExchange::Durable),
               options.testFlag(QAmqpExchange::NoWait));

    frame.setArguments(args);
    sendFrame(frame);
    delayedDeclare = false;
}

bool QAmqpExchangePrivate::_q_method(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpExchange);
    if (QAmqpChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() == QAmqpFrame::Basic) {
        switch (frame.id()) {
        case bmAck:
        case bmNack:
            handleAckOrNack(frame);
            break;
        case bmReturn: basicReturn(frame); break;

        default:
            break;
        }

        return true;
    }

    if (frame.methodClass() == QAmqpFrame::Confirm) {
        if (frame.id() == cmConfirmOk) {
            Q_EMIT q->confirmsEnabled();
            return true;
        }
    }

    if (frame.methodClass() == QAmqpFrame::Exchange) {
        switch (frame.id()) {
        case miDeclareOk: declareOk(frame); break;
        case miDeleteOk: deleteOk(frame); break;

        default:
            break;
        }

        return true;
    }

    return false;
}

void QAmqpExchangePrivate::declareOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAmqpExchange);
    qAmqpDebug("-> exchange[ %s ]#declareOk()", qPrintable(name));
    declared = true;
    Q_EMIT q->declared();
}

void QAmqpExchangePrivate::deleteOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAmqpExchange);
    qAmqpDebug("-> exchange#deleteOk[ %s ]()", qPrintable(name));
    declared = false;
    Q_EMIT q->removed();
}

void QAmqpExchangePrivate::_q_disconnected()
{
    QAmqpChannelPrivate::_q_disconnected();
    qAmqpDebug() << "exchange disconnected: " << name;
    delayedDeclare = false;
    declared = false;
    unconfirmedDeliveryTags.clear();
}

void QAmqpExchangePrivate::basicReturn(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpExchange);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    quint16 replyCode;
    stream >> replyCode;
    QString replyText = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    QString exchangeName = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    QString routingKey = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();

    QAMQP::Error checkError = static_cast<QAMQP::Error>(replyCode);
    if (checkError != QAMQP::NoError) {
        error = checkError;
        errorString = qPrintable(replyText);
        Q_EMIT q->error(error);
    }

    qAmqpDebug("-> basic#return( reply-code=%d, reply-text=%s, exchange=%s, routing-key=%s )",
               replyCode, qPrintable(replyText), qPrintable(exchangeName), qPrintable(routingKey));
}

void QAmqpExchangePrivate::handleAckOrNack(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpExchange);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    qlonglong deliveryTag =
        QAmqpFrame::readAmqpField(stream, QAmqpMetaType::LongLongUint).toLongLong();
    bool multiple = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::Boolean).toBool();
    if (frame.id() == QAmqpExchangePrivate::bmAck) {
        if (deliveryTag == 0) {
            unconfirmedDeliveryTags.clear();
        } else {
            int idx = unconfirmedDeliveryTags.indexOf(deliveryTag);
            if (idx == -1) {
                return;
            }

            if (multiple) {
                unconfirmedDeliveryTags.remove(0, idx + 1);
            } else {
                unconfirmedDeliveryTags.remove(idx);
            }
        }

        if (unconfirmedDeliveryTags.isEmpty())
            Q_EMIT q->allMessagesDelivered();

    } else {
        qAmqpDebug() << "nacked(" << deliveryTag << "), multiple=" << multiple;
    }
}

//////////////////////////////////////////////////////////////////////////

QAmqpExchange::QAmqpExchange(int channelNumber, QAmqpClient *parent)
    : QAmqpChannel(new QAmqpExchangePrivate(this), parent)
{
    Q_D(QAmqpExchange);
    d->init(channelNumber, parent);
}

QAmqpExchange::~QAmqpExchange()
{
}

void QAmqpExchange::channelOpened()
{
    Q_D(QAmqpExchange);
    if (d->delayedDeclare)
        d->declare();

    while (!d->pendingPublishes.isEmpty()) {
        QAmqpExchangePrivate::PendingPublish publish = d->pendingPublishes.dequeue();
        d->pendingPublishBytes -= publish.message.size();
        d->sendPublish(publish);
    }
}

void QAmqpExchange::channelClosed()
{
}

QAmqpExchange::ExchangeOptions QAmqpExchange::options() const
{
    Q_D(const QAmqpExchange);
    return d->options;
}

QString QAmqpExchange::type() const
{
    Q_D(const QAmqpExchange);
    return d->type;
}

bool QAmqpExchange::isDeclared() const
{
    Q_D(const QAmqpExchange);
    return d->declared;
}

void QAmqpExchange::declare(ExchangeType type, ExchangeOptions options, const QAmqpTable &args)
{
    declare(QAmqpExchangePrivate::typeToString(type), options, args);
}

void QAmqpExchange::declare(const QString &type, ExchangeOptions options, const QAmqpTable &args)
{
    Q_D(QAmqpExchange);
    d->type = type;
    d->options = options;
    d->arguments = args;
    d->declare();
}

void QAmqpExchange::remove(int options)
{
    Q_D(QAmqpExchange);
    QAmqpMethodFrame frame(QAmqpFrame::Exchange, QAmqpExchangePrivate::miDelete);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, d->name);
    stream << qint8(options);

    qAmqpDebug("<- exchange#delete( exchange=%s, if-unused=%d, no-wait=%d )",
               qPrintable(d->name), options & QAmqpExchange::roIfUnused, options & QAmqpExchange::roNoWait);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void QAmqpExchange::publish(const QString &message, const QString &routingKey,
                            const QAmqpMessage::PropertyHash &properties, int publishOptions)
{
    publish(message.toUtf8(), routingKey, QLatin1String("text.plain"),
            QAmqpTable(), properties, publishOptions);
}

void QAmqpExchange::publish(const QByteArray &message, const QString &routingKey,
                            const QString &mimeType, const QAmqpMessage::PropertyHash &properties,
                            int publishOptions)
{
    publish(message, routingKey, mimeType, QAmqpTable(), properties, publishOptions);
}

void QAmqpExchange::publish(const QByteArray &message, const QString &routingKey,
                            const QString &mimeType, const QAmqpTable &headers,
                            const QAmqpMessage::PropertyHash &properties, int publishOptions)
{
    Q_D(QAmqpExchange);
    QAmqpExchangePrivate::PendingPublish publish;
    publish.message = message;
    publish.routingKey = routingKey;
    publish.mimeType = mimeType;
    publish.headers = headers;
    publish.properties = properties;
    publish.publishOptions = publishOptions;

    if (!d->opened) {
        // Bound pending publishes so a permanently unavailable broker cannot exhaust memory.
        if (d->pendingPublishes.size() >= 1000 ||
            d->pendingPublishBytes + message.size() > 16 * 1024 * 1024) {
            qAmqpDebug() << Q_FUNC_INFO << "pending publish limit reached, dropping message";
            return;
        }

        d->pendingPublishes.enqueue(publish);
        d->pendingPublishBytes += message.size();
        return;
    }

    d->sendPublish(publish);
}

void QAmqpExchangePrivate::sendPublish(const PendingPublish &publish)
{
    if (nextDeliveryTag > 0) {
        unconfirmedDeliveryTags.append(nextDeliveryTag);
        nextDeliveryTag++;
    }

    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpExchangePrivate::bmPublish);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, name);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, publish.routingKey);
    out << qint8(publish.publishOptions);

    qAmqpDebug("<- basic#publish( exchange=%s, routing-key=%s, mandatory=%d, immediate=%d )",
               qPrintable(name), qPrintable(publish.routingKey),
               publish.publishOptions & QAmqpExchange::poMandatory,
               publish.publishOptions & QAmqpExchange::poImmediate);

    frame.setArguments(arguments);
    sendFrame(frame);

    QAmqpContentFrame content(QAmqpFrame::Basic);
    content.setChannel(channelNumber);
    content.setProperty(QAmqpMessage::ContentType, publish.mimeType);
    content.setProperty(QAmqpMessage::ContentEncoding, "utf-8");
    content.setProperty(QAmqpMessage::Headers, publish.headers);

    QAmqpMessage::PropertyHash::ConstIterator it;
    QAmqpMessage::PropertyHash::ConstIterator itEnd = publish.properties.constEnd();
    for (it = publish.properties.constBegin(); it != itEnd; ++it)
        content.setProperty(it.key(), it.value());
    content.setBodySize(publish.message.size());
    sendFrame(content);

    int fullSize = publish.message.size();
    for (int sent = 0; sent < fullSize; sent += (client->frameMax() - 7)) {
        QAmqpContentBodyFrame body;
        QByteArray partition = publish.message.mid(sent, (client->frameMax() - 7));
        body.setChannel(channelNumber);
        body.setBody(partition);
        sendFrame(body);
    }
}

void QAmqpExchange::enableConfirms(bool noWait)
{
    Q_D(QAmqpExchange);
    QAmqpMethodFrame frame(QAmqpFrame::Confirm, QAmqpExchangePrivate::cmConfirm);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);
    stream << qint8(noWait ? 1 : 0);

    frame.setArguments(arguments);
    d->sendFrame(frame);

    // for tracking acks and nacks
    if (d->nextDeliveryTag == 0) d->nextDeliveryTag = 1;
}

bool QAmqpExchange::waitForConfirms(int msecs)
{
    Q_D(QAmqpExchange);

    QEventLoop loop;
    connect(this, SIGNAL(allMessagesDelivered()), &loop, SLOT(quit()));
    QTimer::singleShot(msecs, &loop, SLOT(quit()));
    loop.exec();

    return (d->unconfirmedDeliveryTags.isEmpty());
}
