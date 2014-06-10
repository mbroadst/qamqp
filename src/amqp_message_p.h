#ifndef amqp_message_p_h__
#define amqp_message_p_h__

#include <QHash>
#include <QSharedData>

#include "amqp_frame.h"
#include "amqp_message.h"

namespace QAMQP {

class MessagePrivate : public QSharedData
{
public:
    MessagePrivate();

    qlonglong deliveryTag;
    bool redelivered;
    QString exchangeName;
    QString routingKey;

    QByteArray payload;
    MessageProperties properties;
    Frame::TableField headers;

    int leftSize;

};

}   // namespace QAMQP

#endif  // amqp_message_p_h__
