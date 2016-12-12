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
#ifndef QAMQPCHANNEL_H
#define QAMQPCHANNEL_H

#include <QObject>
#include "qamqpglobal.h"

class QAmqpClient;
class QAmqpChannelPrivate;
class QAMQP_EXPORT QAmqpChannel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int number READ channelNumber CONSTANT)
    Q_PROPERTY(bool open READ isOpen CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName)
public:
    virtual ~QAmqpChannel();

    int channelNumber() const;
    bool isOpen() const;

    QString name() const;
    void setName(const QString &name);

    QAMQP::Error error() const;
    QString errorString() const;

    qint32 prefetchSize() const;
    qint16 prefetchCount() const;

    void reset();

    // AMQP Basic
    void qos(qint16 prefetchCount, qint32 prefetchSize = 0);

public Q_SLOTS:
    void close();
    void reopen();
    void resume();

Q_SIGNALS:
    void opened();
    void closed();
    void resumed();
    void paused();
    void error(QAMQP::Error error);
    void qosDefined();

protected:
    virtual void channelOpened() = 0;
    virtual void channelClosed() = 0;

protected:
    explicit QAmqpChannel(QAmqpChannelPrivate *dd, QAmqpClient *client);

    Q_DISABLE_COPY(QAmqpChannel)
    Q_DECLARE_PRIVATE(QAmqpChannel)
    QScopedPointer<QAmqpChannelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_open())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())

    friend class QAmqpClientPrivate;
    friend class QAmqpExchangePrivate;
};

#endif  // QAMQPCHANNEL_H
