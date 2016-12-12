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
#ifndef QAMQPQUEUE_H
#define QAMQPQUEUE_H

#include <QQueue>

#include "qamqpchannel.h"
#include "qamqpmessage.h"
#include "qamqpglobal.h"
#include "qamqptable.h"

class QAmqpClient;
class QAmqpClientPrivate;
class QAmqpExchange;
class QAmqpQueuePrivate;
class QAMQP_EXPORT QAmqpQueue : public QAmqpChannel, public QQueue<QAmqpMessage>
{
    Q_OBJECT
    Q_ENUMS(QueueOptions)
    Q_PROPERTY(int options READ options CONSTANT)
    Q_PROPERTY(QString consumerTag READ consumerTag WRITE setConsumerTag)
    Q_ENUMS(QueueOption)
    Q_ENUMS(ConsumeOption)
    Q_ENUMS(RemoveOption)

public:
    enum QueueOption {
        NoOptions = 0x0,
        Passive = 0x01,
        Durable = 0x02,
        Exclusive = 0x04,
        AutoDelete = 0x08,
        NoWait = 0x10
    };
    Q_DECLARE_FLAGS(QueueOptions, QueueOption)
    int options() const;

    enum ConsumeOption {
        coNoLocal = 0x01,
        coNoAck = 0x02,
        coExclusive = 0x04,
        coNoWait = 0x08
    };
    Q_DECLARE_FLAGS(ConsumeOptions, ConsumeOption)

    enum RemoveOption {
        roForce = 0x0,
        roIfUnused = 0x01,
        roIfEmpty = 0x02,
        roNoWait = 0x04
    };
    Q_DECLARE_FLAGS(RemoveOptions, RemoveOption)

    ~QAmqpQueue();

    bool isConsuming() const;
    bool isDeclared() const;

    void setConsumerTag(const QString &consumerTag);
    QString consumerTag() const;

    qint32 messageCount() const;
    qint32 consumerCount() const;

Q_SIGNALS:
    void declared();
    void bound();
    void unbound();
    void removed();
    void purged(int messageCount);

    void messageReceived();
    void empty();
    void consuming(const QString &consumerTag);
    void cancelled(const QString &consumerTag);

public Q_SLOTS:
    // AMQP Queue
    void declare(int options = Durable|AutoDelete, const QAmqpTable &arguments = QAmqpTable());
    void bind(const QString &exchangeName, const QString &key);
    void bind(QAmqpExchange *exchange, const QString &key);
    void unbind(const QString &exchangeName, const QString &key);
    void unbind(QAmqpExchange *exchange, const QString &key);
    void purge();
    void remove(int options = roIfUnused|roIfEmpty|roNoWait);

    // AMQP Basic
    bool consume(int options = NoOptions);
    void get(bool noAck = true);
    bool cancel(bool noWait = false);
    void ack(const QAmqpMessage &message);
    void ack(qlonglong deliveryTag, bool multiple);
    void reject(const QAmqpMessage &message, bool requeue);
    void reject(qlonglong deliveryTag, bool requeue);

protected:
    // reimp Channel
    virtual void channelOpened();
    virtual void channelClosed();

private:
    explicit QAmqpQueue(int channelNumber = -1, QAmqpClient *parent = 0);

    Q_DISABLE_COPY(QAmqpQueue)
    Q_DECLARE_PRIVATE(QAmqpQueue)
    friend class QAmqpClient;
    friend class QAmqpClientPrivate;

};

#endif  // QAMQPQUEUE_H
