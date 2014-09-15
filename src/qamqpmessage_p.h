#ifndef QAMQPMESSAGE_P_H
#define QAMQPMESSAGE_P_H

#include <QHash>
#include <QSharedData>

#include "qamqpframe_p.h"
#include "qamqpmessage.h"

class QAmqpMessagePrivate : public QSharedData
{
public:
    QAmqpMessagePrivate();

    qlonglong deliveryTag;
    bool redelivered;
    QString exchangeName;
    QString routingKey;
    QByteArray payload;
    QHash<QAmqpMessage::Property, QVariant> properties;
    QHash<QString, QVariant> headers;
    int leftSize;

};

#endif  // QAMQPMESSAGE_P_H
