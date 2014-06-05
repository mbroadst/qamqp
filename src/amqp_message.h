#ifndef amqp_message_h__
#define amqp_message_h__

#include <QByteArray>
#include <QHash>
#include <QExplicitlySharedDataPointer>

#include "amqp_frame.h"
#include "amqp_global.h"

namespace QAMQP
{

class MessagePrivate;
class QAMQP_EXPORT Message
{
public:
    Message();
    Message(const Message &other);
    Message &operator=(const Message &other);
    ~Message();

    typedef Frame::Content::Property MessageProperty;
    Q_DECLARE_FLAGS(MessageProperties, MessageProperty)

    qlonglong deliveryTag() const;
    bool redelivered() const;
    QString exchangeName() const;
    QString routingKey() const;
    QByteArray payload() const;
    QHash<MessageProperty, QVariant> properties() const;
    Frame::TableField headers() const;

private:
    QExplicitlySharedDataPointer<MessagePrivate> d;
    friend class QueuePrivate;
    friend class Queue;

};

} // namespace QAMQP

#endif // amqp_message_h__
