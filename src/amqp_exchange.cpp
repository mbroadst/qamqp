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
        d->declare();
}

void Exchange::onClose()
{
    Q_D(Exchange);
    d->remove(true, true);
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

void Exchange::declare(const QString &type, ExchangeOptions option , const Frame::TableField &arg)
{
    Q_D(Exchange);
    d->options = option;
    d->type = type;
    d->arguments = arg;
    d->declare();
}

void Exchange::remove(bool ifUnused, bool noWait)
{
    Q_D(Exchange);
    d->remove(ifUnused, noWait);
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

void Exchange::publish(const QString &message, const QString &key, const MessageProperties &prop)
{
    Q_D(Exchange);
    d->publish(message.toUtf8(), key, QLatin1String("text.plain"), QVariantHash(), prop);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QString &mimeType, const MessageProperties &prop)
{
    Q_D(Exchange);
    d->publish(message, key, mimeType, QVariantHash(), prop);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QVariantHash &headers, const QString &mimeType,
                       const MessageProperties &prop)
{
    Q_D(Exchange);
    d->publish(message, key, mimeType, headers, prop);
}

//////////////////////////////////////////////////////////////////////////

ExchangePrivate::ExchangePrivate(Exchange *q)
    : ChannelPrivate(q),
      delayedDeclare(false),
      declared(false)
{
}

ExchangePrivate::~ExchangePrivate()
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
    QMetaObject::invokeMethod(q, "declared");
}

void ExchangePrivate::deleteOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Exchange);
    qDebug() << "Deleted exchange: " << name;
    declared = false;
    QMetaObject::invokeMethod(q, "removed");
}

void ExchangePrivate::declare()
{
    if (!opened) {
        delayedDeclare = true;
        return;
    }

    if (name.isEmpty())
        return;

    Frame::Method frame(Frame::fcExchange, miDeclare);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    stream << qint16(0); //reserver 1
    Frame::writeField('s', stream, name);
    Frame::writeField('s', stream, type);
    stream << qint8(options);
    Frame::writeField('F', stream, ExchangePrivate::arguments);

    frame.setArguments(arguments_);
    sendFrame(frame);
    delayedDeclare = false;
}

void ExchangePrivate::remove(bool ifUnused, bool noWait)
{
    Frame::Method frame(Frame::fcExchange, miDelete);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    stream << qint16(0); //reserver 1
    Frame::writeField('s', stream, name);

    qint8 flag = 0;

    flag |= (ifUnused ? 0x1 : 0);
    flag |= (noWait ? 0x2 : 0);

    stream << flag; //reserver 1

    frame.setArguments(arguments_);
    sendFrame(frame);
}

void ExchangePrivate::publish(const QByteArray &message, const QString &key,
                              const QString &mimeType, const QVariantHash &headers,
                              const Exchange::MessageProperties &prop)
{
    Frame::Method frame(Frame::fcBasic, bmPublish);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream out(&arguments_, QIODevice::WriteOnly);

    out << qint16(0); //reserver 1
    Frame::writeField('s', out, name);
    Frame::writeField('s', out, key);
    out << qint8(0);

    frame.setArguments(arguments_);
    sendFrame(frame);

    Frame::Content content(Frame::fcBasic);
    content.setChannel(number);
    content.setProperty(Frame::Content::cpContentType, mimeType);
    content.setProperty(Frame::Content::cpContentEncoding, "utf-8");
    content.setProperty(Frame::Content::cpHeaders, headers);
    content.setProperty(Frame::Content::cpMessageId, "0");

    Exchange::MessageProperties::ConstIterator it;
    Exchange::MessageProperties::ConstIterator itEnd = prop.constEnd();
    for (it = prop.constBegin(); it != itEnd; ++it)
        content.setProperty(it.key(), it.value());

    content.setBody(message);
    sendFrame(content);

    int fullSize = message.size();
    for (int sended_ = 0; sended_ < fullSize; sended_+= (FRAME_MAX - 7)) {
        Frame::ContentBody body;
        QByteArray partition_ = message.mid(sended_, (FRAME_MAX - 7));
        body.setChannel(number);
        body.setBody(partition_);
        sendFrame(body);
    }
}

void ExchangePrivate::_q_disconnected()
{
    ChannelPrivate::_q_disconnected();
    qDebug() << "Exchange " << name << " disconnected";
    delayedDeclare = false;
    declared = false;
}
