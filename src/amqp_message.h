#ifndef amqp_message_h__
#define amqp_message_h__

#include "amqp_frame.h"
#include <QByteArray>
#include <QHash>
#include <QSharedPointer>

namespace QAMQP
{

struct Message
{
    Message();
    virtual ~Message();

    typedef Frame::Content::Property MessageProperty;
    Q_DECLARE_FLAGS(MessageProperties, MessageProperty)

    qlonglong deliveryTag;
    QByteArray payload;
    QHash<MessageProperty, QVariant> property;
    Frame::TableField headers;
    QString routeKey;
    QString exchangeName;
    int leftSize;
};

typedef QSharedPointer<Message> MessagePtr;

} // namespace QAMQP

#endif // amqp_message_h__
