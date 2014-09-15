#include <QDataStream>
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
      declared(false)
{
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

    QAmqpMethodFrame frame(QAmqpFrame::fcExchange, QAmqpExchangePrivate::miDeclare);
    frame.setChannel(channelNumber);

    QByteArray args;
    QDataStream stream(&args, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, name);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, type);

    stream << qint8(options);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::Hash, arguments);

    frame.setArguments(args);
    sendFrame(frame);
    delayedDeclare = false;
}

bool QAmqpExchangePrivate::_q_method(const QAmqpMethodFrame &frame)
{
    if (QAmqpChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() == QAmqpFrame::fcExchange) {
        switch (frame.id()) {
        case miDeclareOk:
            declareOk(frame);
            break;

        case miDeleteOk:
            deleteOk(frame);
            break;

        default:
            break;
        }

        return true;
    } else if (frame.methodClass() == QAmqpFrame::fcBasic) {
        switch (frame.id()) {
        case bmReturn:
            basicReturn(frame);
            break;

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
    qAmqpDebug() << "declared exchange: " << name;
    declared = true;
    Q_EMIT q->declared();
}

void QAmqpExchangePrivate::deleteOk(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)

    Q_Q(QAmqpExchange);
    qAmqpDebug() << "deleted exchange: " << name;
    declared = false;
    Q_EMIT q->removed();
}

void QAmqpExchangePrivate::_q_disconnected()
{
    QAmqpChannelPrivate::_q_disconnected();
    qAmqpDebug() << "exchange " << name << " disconnected";
    delayedDeclare = false;
    declared = false;
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

    qAmqpDebug(">> replyCode: %d", replyCode);
    qAmqpDebug(">> replyText: %s", qPrintable(replyText));
    qAmqpDebug(">> exchangeName: %s", qPrintable(exchangeName));
    qAmqpDebug(">> routingKey: %s", qPrintable(routingKey));
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
    QAmqpMethodFrame frame(QAmqpFrame::fcExchange, QAmqpExchangePrivate::miDelete);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, d->name);
    stream << qint8(options);

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
    QAmqpMethodFrame frame(QAmqpFrame::fcBasic, QAmqpExchangePrivate::bmPublish);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, d->name);
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, routingKey);
    out << qint8(publishOptions);

    frame.setArguments(arguments);
    d->sendFrame(frame);

    QAmqpContentFrame content(QAmqpFrame::fcBasic);
    content.setChannel(d->channelNumber);
    content.setProperty(QAmqpMessage::ContentType, mimeType);
    content.setProperty(QAmqpMessage::ContentEncoding, "utf-8");
    content.setProperty(QAmqpMessage::Headers, headers);
    content.setProperty(QAmqpMessage::MessageId, "0");

    QAmqpMessage::PropertyHash::ConstIterator it;
    QAmqpMessage::PropertyHash::ConstIterator itEnd = properties.constEnd();
    for (it = properties.constBegin(); it != itEnd; ++it)
        content.setProperty(it.key(), it.value());
    content.setBodySize(message.size());
    d->sendFrame(content);

    int fullSize = message.size();
    for (int sent = 0; sent < fullSize; sent += (d->client->frameMax() - 7)) {
        QAmqpContentBodyFrame body;
        QByteArray partition = message.mid(sent, (d->client->frameMax() - 7));
        body.setChannel(d->channelNumber);
        body.setBody(partition);
        d->sendFrame(body);
    }
}
