#include <QHash>

#include "qamqpmessage.h"
#include "qamqpmessage_p.h"
using namespace QAMQP;

MessagePrivate::MessagePrivate()
    : deliveryTag(0),
      leftSize(0)
{
}

//////////////////////////////////////////////////////////////////////////

Message::Message()
    : d(new MessagePrivate)
{
}

Message::Message(const Message &other)
    : d(other.d)
{
}

Message::~Message()
{
}

Message &Message::operator=(const Message &other)
{
    d = other.d;
    return *this;
}

bool Message::operator==(const Message &message) const
{
    if (message.d == d)
        return true;

    return (message.d->deliveryTag == d->deliveryTag &&
            message.d->redelivered == d->redelivered &&
            message.d->exchangeName == d->exchangeName &&
            message.d->routingKey == d->routingKey &&
            message.d->payload == d->payload &&
            message.d->properties == d->properties &&
            message.d->headers == d->headers &&
            message.d->leftSize == d->leftSize);
}

bool Message::isValid() const
{
    return d->deliveryTag != 0 &&
           !d->exchangeName.isNull() &&
           !d->routingKey.isNull();
}

qlonglong Message::deliveryTag() const
{
    return d->deliveryTag;
}

bool Message::isRedelivered() const
{
    return d->redelivered;
}

QString Message::exchangeName() const
{
    return d->exchangeName;
}

QString Message::routingKey() const
{
    return d->routingKey;
}

QByteArray Message::payload() const
{
    return d->payload;
}

bool Message::hasProperty(Property property) const
{
    return d->properties.contains(property);
}

void Message::setProperty(Property property, const QVariant &value)
{
    d->properties.insert(property, value);
}

QVariant Message::property(Property property, const QVariant &defaultValue) const
{
    return d->properties.value(property, defaultValue);
}

bool Message::hasHeader(const QString &header) const
{
    return d->headers.contains(header);
}

void Message::setHeader(const QString &header, const QVariant &value)
{
    d->headers.insert(header, value);
}

QVariant Message::header(const QString &header, const QVariant &defaultValue) const
{
    return d->headers.value(header, defaultValue);
}

uint qHash(const QAMQP::Message &message, uint seed)
{
    Q_UNUSED(seed);
    return qHash(message.deliveryTag()) ^
           qHash(message.isRedelivered()) ^
           qHash(message.exchangeName()) ^
           qHash(message.routingKey()) ^
           qHash(message.payload());
}
