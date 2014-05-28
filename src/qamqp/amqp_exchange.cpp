#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"

using namespace QAMQP;
using namespace QAMQP::Frame;

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

Exchange::Exchange(int channelNumber, Client *parent)
    : Channel(new ExchangePrivate(this), parent)
{
    Q_D(QAMQP::Exchange);
    d->init(channelNumber, parent);
}

Exchange::~Exchange()
{
    remove();
}

void Exchange::onOpen()
{
    Q_D(QAMQP::Exchange);
    if (d->delayedDeclare)
        d->declare();
}

void Exchange::onClose()
{
    Q_D(QAMQP::Exchange);
    d->remove(true, true);
}

Exchange::ExchangeOptions Exchange::option() const
{
    Q_D(const QAMQP::Exchange);
    return d->options;
}

QString Exchange::type() const
{
    Q_D(const QAMQP::Exchange);
    return d->type;
}

void Exchange::declare(const QString &type, ExchangeOptions option , const TableField &arg)
{
    Q_D(QAMQP::Exchange);
    d->options = option;
    d->type = type;
    d->arguments = arg;
    d->declare();
}

void Exchange::remove(bool ifUnused, bool noWait)
{
    Q_D(QAMQP::Exchange);
    d->remove(ifUnused, noWait);
}

void Exchange::bind(QAMQP::Queue *queue)
{
    Q_D(QAMQP::Exchange);
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
    Q_D(QAMQP::Exchange);
    d->publish(message.toUtf8(), key, QLatin1String("text.plain"), QVariantHash(), prop);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QString &mimeType, const MessageProperties &prop)
{
    Q_D(QAMQP::Exchange);
    d->publish(message, key, mimeType, QVariantHash(), prop);
}

void Exchange::publish(const QByteArray &message, const QString &key,
                       const QVariantHash &headers, const QString &mimeType,
                       const MessageProperties &prop)
{
    Q_D(QAMQP::Exchange);
    d->publish(message, key, mimeType, headers, prop);
}

//////////////////////////////////////////////////////////////////////////

ExchangePrivate::ExchangePrivate(Exchange * q)
    : ChannelPrivate(q),
      delayedDeclare(false),
      declared(false)
{
}

ExchangePrivate::~ExchangePrivate()
{
}

bool ExchangePrivate::_q_method(const QAMQP::Frame::Method &frame)
{
    if (ChannelPrivate::_q_method(frame))
        return true;

    if (frame.methodClass() != QAMQP::Frame::fcExchange)
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

void ExchangePrivate::declareOk(const QAMQP::Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAMQP::Exchange);
    qDebug() << "Declared exchange: " << name;
    declared = true;
    QMetaObject::invokeMethod(q, "declared");
}

void ExchangePrivate::deleteOk(const QAMQP::Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAMQP::Exchange);
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

    QAMQP::Frame::Method frame(QAMQP::Frame::fcExchange, miDeclare);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    stream << qint16(0); //reserver 1
    writeField('s', stream, name);
    writeField('s', stream, type);
    stream << qint8(options);
    writeField('F', stream, ExchangePrivate::arguments);

    frame.setArguments(arguments_);
    sendFrame(frame);
    delayedDeclare = false;
}

void ExchangePrivate::remove(bool ifUnused, bool noWait)
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcExchange, miDelete);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    stream << qint16(0); //reserver 1
    writeField('s', stream, name);

    qint8 flag = 0;

    flag |= (ifUnused ? 0x1 : 0);
    flag |= (noWait ? 0x2 : 0);

    stream << flag; //reserver 1

    frame.setArguments(arguments_);
    sendFrame(frame);
}

void ExchangePrivate::publish(const QByteArray &message, const QString &key,
                              const QString &mimeType, const QVariantHash & headers,
                              const Exchange::MessageProperties &prop)
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, bmPublish);
    frame.setChannel(number);
    QByteArray arguments_;
    QDataStream out(&arguments_, QIODevice::WriteOnly);

    out << qint16(0); //reserver 1
    writeField('s', out, name);
    writeField('s', out, key);
    out << qint8(0);

    frame.setArguments(arguments_);
    sendFrame(frame);

    QAMQP::Frame::Content content(QAMQP::Frame::fcBasic);
    content.setChannel(number);
    content.setProperty(Content::cpContentType, mimeType);
    content.setProperty(Content::cpContentEncoding, "utf-8");
    content.setProperty(Content::cpHeaders, headers);
    content.setProperty(Content::cpMessageId, "0");

    Exchange::MessageProperties::ConstIterator i;

    for (i = prop.begin(); i != prop.end(); ++i)
        content.setProperty(i.key(), i.value());

    content.setBody(message);
    sendFrame(content);

    int fullSize = message.size();
    for (int sended_ = 0; sended_ < fullSize; sended_+= (FRAME_MAX - 7)) {
        QAMQP::Frame::ContentBody body;
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
