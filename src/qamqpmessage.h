#ifndef QAMQPMESSAGE_H
#define QAMQPMESSAGE_H

#include <QByteArray>
#include <QHash>
#include <QSharedDataPointer>
#include <QVariant>

#include "qamqpglobal.h"

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
    typedef QHash<Property, QVariant> PropertyHash;

    bool hasProperty(Property property) const;
    void setProperty(Property property, const QVariant &value);
    QVariant property(Property property, const QVariant &defaultValue = QVariant()) const;

    bool hasHeader(const QString &header) const;
    void setHeader(const QString &header, const QVariant &value);
    QVariant header(const QString &header, const QVariant &defaultValue = QVariant()) const;

    bool isValid() const;
    bool isRedelivered() const;
    qlonglong deliveryTag() const;
    QString exchangeName() const;
    QString routingKey() const;
    QByteArray payload() const;

private:
    QSharedDataPointer<MessagePrivate> d;
    friend class QueuePrivate;
    friend class Queue;

};

} // namespace QAMQP

Q_DECLARE_METATYPE(QAMQP::Message::PropertyHash)

#endif // QAMQPMESSAGE_H
