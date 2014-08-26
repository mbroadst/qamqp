#ifndef QAMQPMESSAGE_P_H
#define QAMQPMESSAGE_P_H

#include <QHash>
#include <QSharedData>

#include "qamqpframe_p.h"
#include "qamqpmessage.h"

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
    QHash<Message::Property, QVariant> properties;
    QHash<QString, QVariant> headers;
    int leftSize;

};

}   // namespace QAMQP

#endif  // QAMQPMESSAGE_P_H
