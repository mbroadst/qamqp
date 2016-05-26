#include <QHash>

#include "qamqpmessage.h"
#include "qamqpmessage_p.h"

QAmqpMessagePrivate::QAmqpMessagePrivate()
    : deliveryTag(0),
      leftSize(0)
{
}

//////////////////////////////////////////////////////////////////////////

QAmqpMessage::QAmqpMessage()
    : d(new QAmqpMessagePrivate)
{
}

QAmqpMessage::QAmqpMessage(const QAmqpMessage &other)
    : d(other.d)
{
}

QAmqpMessage::~QAmqpMessage()
{
}

QAmqpMessage &QAmqpMessage::operator=(const QAmqpMessage &other)
{
    d = other.d;
    return *this;
}

bool QAmqpMessage::operator==(const QAmqpMessage &message) const
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

bool QAmqpMessage::isValid() const
{
    return d->deliveryTag != 0 &&
           !d->exchangeName.isNull() &&
           !d->routingKey.isNull();
}

qlonglong QAmqpMessage::deliveryTag() const
{
    return d->deliveryTag;
}

bool QAmqpMessage::isRedelivered() const
{
    return d->redelivered;
}

QString QAmqpMessage::exchangeName() const
{
    return d->exchangeName;
}

QString QAmqpMessage::routingKey() const
{
    return d->routingKey;
}

QByteArray QAmqpMessage::payload() const
{
    return d->payload;
}

bool QAmqpMessage::hasProperty(Property property) const
{
    return d->properties.contains(property);
}

void QAmqpMessage::setProperty(Property property, const QVariant &value)
{
    d->properties.insert(property, value);
}

QVariant QAmqpMessage::property(Property property, const QVariant &defaultValue) const
{
    return d->properties.value(property, defaultValue);
}

bool QAmqpMessage::hasHeader(const QString &header) const
{
    return d->headers.contains(header);
}

void QAmqpMessage::setHeader(const QString &header, const QVariant &value)
{
    d->headers.insert(header, value);
}

QVariant QAmqpMessage::header(const QString &header, const QVariant &defaultValue) const
{
    return d->headers.value(header, defaultValue);
}

QHash<QString, QVariant> QAmqpMessage::headers() const
{
    return d->headers;
}

#if QT_VERSION < 0x050000
bool QAmqpMessage::isDetached() const
{
    return d && d->ref == 1;
}
#endif

uint qHash(const QAmqpMessage &message, uint seed)
{
    Q_UNUSED(seed);
    return qHash(message.deliveryTag()) ^
           qHash(message.isRedelivered()) ^
           qHash(message.exchangeName()) ^
           qHash(message.routingKey()) ^
           qHash(message.payload());
}
