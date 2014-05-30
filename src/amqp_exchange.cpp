#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"
#include "amqp_global.h"

#include <QDataStream>
#include <QDebug>

using namespace QAMQP;

Exchange::Exchange(int channelNumber, Client *parent)
    : Channel(new ExchangePrivate(this), parent)
{
    Q_D(Exchange);
    d->init(channelNumber, parent);
}

Exchange::~Exchange()
{
    remove();
}

void Exchange::onOpen()
{
    Q_D(Exchange);
    if (d->delayedDeclare)
        declare();
}

void Exchange::onClose()
{
    remove(true, true);
}

Exchange::ExchangeOptions Exchange::option() const
{
    Q_D(const Exchange);
    return d->options;
}

QString Exchange::type() const
{
    Q_D(const Exchange);
    return d->type;
}

void Exchange::declare(const QString &type, ExchangeOptions options , const Frame::TableField &args)
{
    Q_D(Exchange);
    d->type = type;
    d->options = options;
    d->arguments = args;

    if (!d->opened) {
        d->delayedDeclare = true;
        return;
    }

    if (d->name.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "attempting to declare an unnamed exchange, aborting...";
        return;
    }

    Frame::Method frame(Frame::fcExchange, ExchangePrivate::miDeclare);
    frame.setChannel(d->number);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0);    //reserver 1
    Frame::writeField('s', stream, d->name);
    Frame::writeField('s', stream, d->type);

    stream << qint8(d->options);
    Frame::writeField('F', stream, d->arguments);

    frame.setArguments(arguments);
    d->sendFrame(frame);
    d->delayedDeclare = false;
}

void Exchange::remove(bool ifUnused, bool noWait)
{
    Q_D(Exchange);
    Frame::Method frame(Frame::fcExchange, ExchangePrivate::miDelete);
    frame.setChannel(d->number);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0);    //reserver 1
    Frame::writeField('s', stream, d->name);

    qint8 flag = 0;
    flag |= (ifUnused ? 0x1 : 0);
    flag |= (noWait ? 0x2 : 0);
    stream << flag;    //reserver 1

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

void Exchange::bind(Queue *queue)
{
    Q_D(Exchange);
    queue->bind(this, d->name);
}

void Exchange::bind(const QString &queueName)
{
    Q_UNUSED(queueName);
    qWarning("Not implemented");
}

void Exchange::bind(const QString &queueName, const QString &key)
{
    Q_UNUSED(queueName);
    Q_UNUSED(key);
    qWarning("Not implemented");
}

void Exchange::publish(const QString &message, const QString &key,
                       const MessageProperties &properties)
{
    publish(message.toUtf8(), key, QLatin1String("text.plain"), QVariantHash(), properties);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QString &mimeType, const MessageProperties &properties)
{
    publish(message, key, mimeType, QVariantHash(), properties);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QString &mimeType, const QVariantHash &headers,
                       const Exchange::MessageProperties &properties)
{
    Q_D(Exchange);
    Frame::Method frame(Frame::fcBasic, ExchangePrivate::bmPublish);
    frame.setChannel(d->number);

    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << qint16(0);   //reserver 1
    Frame::writeField('s', out, d->name);
    Frame::writeField('s', out, key);
    out << qint8(0);

    frame.setArguments(arguments);
    d->sendFrame(frame);

    Frame::Content content(Frame::fcBasic);
    content.setChannel(d->number);
    content.setProperty(Frame::Content::cpContentType, mimeType);
    content.setProperty(Frame::Content::cpContentEncoding, "utf-8");
    content.setProperty(Frame::Content::cpHeaders, headers);
    content.setProperty(Frame::Content::cpMessageId, "0");

    Exchange::MessageProperties::ConstIterator it;
    Exchange::MessageProperties::ConstIterator itEnd = properties.constEnd();
    for (it = properties.constBegin(); it != itEnd; ++it)
        content.setProperty(it.key(), it.value());
    content.setBody(message);
    d->sendFrame(content);

    int fullSize = message.size();
    for (int sent = 0; sent < fullSize; sent += (FRAME_MAX - 7)) {
        Frame::ContentBody body;
        QByteArray partition = message.mid(sent, (FRAME_MAX - 7));
        body.setChannel(d->number);
        body.setBody(partition);
        d->sendFrame(body);
    }
}

//////////////////////////////////////////////////////////////////////////

ExchangePrivate::ExchangePrivate(Exchange *q)
    : ChannelPrivate(q),
      delayedDeclare(false),
      declared(false)
{
}

bool ExchangePrivate::_q_method(const Frame::Method &frame)
{
    if (ChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() != Frame::fcExchange)
        return false;

    switch(frame.id()) {
    case miDeclareOk:
        declareOk(frame);
        break;
    case miDelete:
        deleteOk(frame);
        break;
    default:
        break;
    }

    return true;
}

void ExchangePrivate::declareOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Exchange);
    qDebug() << "Declared exchange: " << name;
    declared = true;
    Q_EMIT q->declared();
}

void ExchangePrivate::deleteOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)

    Q_Q(Exchange);
    qDebug() << "Deleted exchange: " << name;
    declared = false;
    Q_EMIT q->removed();
}

void ExchangePrivate::_q_disconnected()
{
    ChannelPrivate::_q_disconnected();
    qDebug() << "Exchange " << name << " disconnected";
    delayedDeclare = false;
    declared = false;
}
