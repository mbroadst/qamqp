#ifndef amqp_message_h__
#define amqp_message_h__

#include <QByteArray>
#include <QHash>
#include <QSharedDataPointer>

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

    enum Property {
        ContentType = AMQP_BASIC_CONTENT_TYPE_FLAG,
        ContentEncoding = AMQP_BASIC_CONTENT_ENCODING_FLAG,
        Headers = AMQP_BASIC_HEADERS_FLAG,
        DeliveryMode = AMQP_BASIC_DELIVERY_MODE_FLAG,
        Priority = AMQP_BASIC_PRIORITY_FLAG,
        CorrelationId = AMQP_BASIC_CORRELATION_ID_FLAG,
        ReplyTo = AMQP_BASIC_REPLY_TO_FLAG,
        Expiration = AMQP_BASIC_EXPIRATION_FLAG,
        MessageId = AMQP_BASIC_MESSAGE_ID_FLAG,
        Timestamp = AMQP_BASIC_TIMESTAMP_FLAG,
        Type = AMQP_BASIC_TYPE_FLAG,
        UserId = AMQP_BASIC_USER_ID_FLAG,
        AppId = AMQP_BASIC_APP_ID_FLAG,
        ClusterID = AMQP_BASIC_CLUSTER_ID_FLAG
    };
    Q_DECLARE_FLAGS(Properties, Property)

    bool hasProperty(Property property) const;
    void setProperty(Property property, const QVariant &value);
    QVariant property(Property property, const QVariant &defaultValue = QString()) const;

    bool isValid() const;

    qlonglong deliveryTag() const;
    bool redelivered() const;
    QString exchangeName() const;
    QString routingKey() const;
    QByteArray payload() const;
    Frame::TableField headers() const;

private:
    QSharedDataPointer<MessagePrivate> d;
    friend class QueuePrivate;
    friend class Queue;

};

} // namespace QAMQP

#endif // amqp_message_h__
