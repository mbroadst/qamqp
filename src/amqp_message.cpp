#include "amqp_message.h"
#include "amqp_message_p.h"

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

qlonglong Message::deliveryTag() const
{
    return d->deliveryTag;
}

bool Message::redelivered() const
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

QHash<Message::MessageProperty, QVariant> Message::properties() const
{
    return d->properties;
}

Frame::TableField Message::headers() const
{
    return d->headers;
}
