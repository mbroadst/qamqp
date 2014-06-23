#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"
#include "amqp_global.h"
#include "amqp_client.h"

#include <QDataStream>
#include <QDebug>

using namespace QAMQP;


QString ExchangePrivate::typeToString(Exchange::ExchangeType type)
{
    switch (type) {
    case Exchange::Direct: return QLatin1String("direct");
    case Exchange::FanOut: return QLatin1String("fanout");
    case Exchange::Topic: return QLatin1String("topic");
    case Exchange::Headers: return QLatin1String("headers");
    }

    return QLatin1String("direct");
}

ExchangePrivate::ExchangePrivate(Exchange *q)
    : ChannelPrivate(q),
      delayedDeclare(false),
      declared(false)
{
}

void ExchangePrivate::declare()
{
    if (!opened) {
        delayedDeclare = true;
        return;
    }

    if (name.isEmpty()) {
        qAmqpDebug() << Q_FUNC_INFO << "attempting to declare an unnamed exchange, aborting...";
        return;
    }

    Frame::Method frame(Frame::fcExchange, ExchangePrivate::miDeclare);
    frame.setChannel(channelNumber);

    QByteArray args;
    QDataStream stream(&args, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    Frame::writeField('s', stream, name);
    Frame::writeField('s', stream, type);

    stream << qint8(options);
    Frame::writeField('F', stream, arguments);

    frame.setArguments(args);
    sendFrame(frame);
    delayedDeclare = false;
}

bool ExchangePrivate::_q_method(const Frame::Method &frame)
{
    if (ChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() == Frame::fcExchange) {
        switch (frame.id()) {
        case miDeclareOk:
            declareOk(frame);
            break;

        case miDeleteOk:
            deleteOk(frame);
            break;

        default:
            qDebug() << Q_FUNC_INFO << "unhandled exchange method: " << frame.id();
            break;
        }

        return true;
    } else if (frame.methodClass() == Frame::fcBasic) {
        switch (frame.id()) {
        case bmReturn:
            basicReturn(frame);
            break;

        default:
            qDebug() << Q_FUNC_INFO << "unhandled basic method: " << frame.id();
            break;
        }

        return true;
    }

    return false;
}

void ExchangePrivate::declareOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Exchange);
    qAmqpDebug() << "declared exchange: " << name;
    declared = true;
    Q_EMIT q->declared();
}

void ExchangePrivate::deleteOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Exchange);
    qAmqpDebug() << "deleted exchange: " << name;
    declared = false;
    Q_EMIT q->removed();
}

void ExchangePrivate::_q_disconnected()
{
    ChannelPrivate::_q_disconnected();
    qAmqpDebug() << "exchange " << name << " disconnected";
    delayedDeclare = false;
    declared = false;
}

void ExchangePrivate::basicReturn(const Frame::Method &frame)
{
    Q_Q(Exchange);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    quint16 replyCode;
    stream >> replyCode;
    QString replyText = Frame::readField('s', stream).toString();
    QString exchangeName = Frame::readField('s', stream).toString();
    QString routingKey = Frame::readField('s', stream).toString();

    Error checkError = static_cast<Error>(replyCode);
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

Exchange::Exchange(int channelNumber, Client *parent)
    : Channel(new ExchangePrivate(this), parent)
{
    Q_D(Exchange);
    d->init(channelNumber, parent);
}

Exchange::~Exchange()
{
}

void Exchange::channelOpened()
{
    Q_D(Exchange);
    if (d->delayedDeclare)
        d->declare();
}

void Exchange::channelClosed()
{
}

Exchange::ExchangeOptions Exchange::options() const
{
    Q_D(const Exchange);
    return d->options;
}

QString Exchange::type() const
{
    Q_D(const Exchange);
    return d->type;
}

void Exchange::declare(ExchangeType type, ExchangeOptions options , const Frame::TableField &args)
{
    declare(ExchangePrivate::typeToString(type), options, args);
}

void Exchange::declare(const QString &type, ExchangeOptions options , const Frame::TableField &args)
{
    Q_D(Exchange);
    d->type = type;
    d->options = options;
    d->arguments = args;
    d->declare();
}

void Exchange::remove(int options)
{
    Q_D(Exchange);
    Frame::Method frame(Frame::fcExchange, ExchangePrivate::miDelete);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0);    //reserved 1
    Frame::writeField('s', stream, d->name);
    stream << qint8(options);

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Exchange::publish(const QString &message, const QString &routingKey,
                       const MessageProperties &properties, int publishOptions)
{
    publish(message.toUtf8(), routingKey, QLatin1String("text.plain"),
            QVariantHash(), properties, publishOptions);
}

void Exchange::publish(const QByteArray &message, const QString &routingKey,
                       const QString &mimeType, const MessageProperties &properties,
                       int publishOptions)
{
    publish(message, routingKey, mimeType, QVariantHash(), properties, publishOptions);
}

void Exchange::publish(const QByteArray &message, const QString &routingKey,
                       const QString &mimeType, const QVariantHash &headers,
                       const MessageProperties &properties, int publishOptions)
{
    Q_D(Exchange);
    Frame::Method frame(Frame::fcBasic, ExchangePrivate::bmPublish);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserved 1
    Frame::writeField('s', out, d->name);
    Frame::writeField('s', out, routingKey);
    out << qint8(publishOptions);

    frame.setArguments(arguments);
    d->sendFrame(frame);

    Frame::Content content(Frame::fcBasic);
    content.setChannel(d->channelNumber);
    content.setProperty(Frame::Content::cpContentType, mimeType);
    content.setProperty(Frame::Content::cpContentEncoding, "utf-8");
    content.setProperty(Frame::Content::cpHeaders, headers);
    content.setProperty(Frame::Content::cpMessageId, "0");

    MessageProperties::ConstIterator it;
    MessageProperties::ConstIterator itEnd = properties.constEnd();
    for (it = properties.constBegin(); it != itEnd; ++it)
        content.setProperty(it.key(), it.value());
    content.setBody(message);
    d->sendFrame(content);

    int fullSize = message.size();
    for (int sent = 0; sent < fullSize; sent += (d->client->frameMax() - 7)) {
        Frame::ContentBody body;
        QByteArray partition = message.mid(sent, (d->client->frameMax() - 7));
        body.setChannel(d->channelNumber);
        body.setBody(partition);
        d->sendFrame(body);
    }
}
