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
#ifndef QAMQPCHANNELHASH_P_H
#define QAMQPCHANNELHASH_P_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QObject>

/* Forward declarations */
class QAmqpChannel;
class QAmqpQueue;
class QAmqpExchange;

/*!
 * QAmqpChannelHash is a container for storing queues and exchanges for later
 * retrieval.  When the objects are destroyed, they are automatically removed
 * from the container.
 */
class QAmqpChannelHash : public QObject
{
    Q_OBJECT
public:
    /*!
     * Retrieve a pointer to the named channel.
     *
     * A NULL string is assumed to be equivalent to "" for the purpose
     * of retrieving the nameless (default) exchange.
     *
     * \param[in]   name    The name of the channel to retrieve.
     * \retval      NULL    Channel does not exist.
     */
    QAmqpChannel* get(const QString& name) const;

    /*!
     * Return true if the named channel exists.
     */
    bool contains(const QString& name) const;

    /**
     * Returns a list of channels tracked by this hash
     */
    QStringList channels() const;

    /*!
     * Store an exchange in the hash.  The nameless exchange is stored under
     * the name "".
     */
    void put(QAmqpExchange* exchange);

    /*!
     * Store a queue in the hash.  If the queue is nameless, we hook its
     * declared signal and store it when the queue receives a name from the
     * broker, otherwise we store it under the name given.
     */
    void put(QAmqpQueue* queue);

private Q_SLOTS:
    /*!
     * Handle destruction of a channel.  Do a full garbage collection run.
     */
    void channelDestroyed(QObject* object);

    /*!
     * Handle a queue that has just been declared and given a new name.  The
     * caller is assumed to be a QAmqpQueue instance.
     */
    void queueDeclared();

private:
    /*!
     * Store a channel in the hash.  This hooks the 'destroyed' signal
     * so the channel can be removed from our list.
     */
    void put(const QString& name, QAmqpChannel* channel);

    /*! A collection of channels.  Key is the channel's "name". */
    QHash<QString, QAmqpChannel*> m_channels;
};

/* vim: set ts=4 sw=4 et */
#endif
