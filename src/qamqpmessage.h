/*
 * Copyright (C) 2012-2014 Alexey Shcherbakov
 * Copyright (C) 2014-2015 Matt Broadstone
 * Contact: https://github.com/mbroadst/qamqp
 *
 * This file is part of the QAMQP Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#ifndef QAMQPMESSAGE_H
#define QAMQPMESSAGE_H

#include <QByteArray>
#include <QHash>
#include <QSharedDataPointer>
#include <QVariant>

#include "qamqpglobal.h"

class QAmqpMessagePrivate;
class QAMQP_EXPORT QAmqpMessage
{
public:
    QAmqpMessage();
    QAmqpMessage(const QAmqpMessage &other);
    QAmqpMessage &operator=(const QAmqpMessage &other);
    ~QAmqpMessage();

#if QT_VERSION >= 0x050000
    inline void swap(QAmqpMessage &other) { qSwap(d, other.d); }
#endif

    bool operator==(const QAmqpMessage &message) const;
    inline bool operator!=(const QAmqpMessage &message) const { return !(operator==(message)); }

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
    QHash<QString, QVariant> headers() const;

    bool isValid() const;
    bool isRedelivered() const;
    qlonglong deliveryTag() const;
    QString exchangeName() const;
    QString routingKey() const;
    QByteArray payload() const;

private:
    QSharedDataPointer<QAmqpMessagePrivate> d;
    friend class QAmqpQueuePrivate;
    friend class QAmqpQueue;

#if QT_VERSION < 0x050000
public:
    typedef QSharedDataPointer<QAmqpMessagePrivate> DataPtr;
    inline DataPtr &data_ptr() { return d; }

    // internal
    bool isDetached() const;
#endif
};

Q_DECLARE_METATYPE(QAmqpMessage::PropertyHash)

#if QT_VERSION < 0x050000
Q_DECLARE_TYPEINFO(QAmqpMessage, Q_MOVABLE_TYPE);
#endif
Q_DECLARE_SHARED(QAmqpMessage)

// NOTE: needed only for MSVC support, don't depend on this hash
QAMQP_EXPORT uint qHash(const QAmqpMessage &key, uint seed = 0);

#endif // QAMQPMESSAGE_H
