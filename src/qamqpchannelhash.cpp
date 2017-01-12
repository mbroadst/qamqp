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
#include "qamqpqueue.h"
#include "qamqpexchange.h"
#include "qamqpchannelhash_p.h"

/*!
* Retrieve a pointer to the named channel.
*
* A NULL string is assumed to be equivalent to "" for the purpose
* of retrieving the nameless (default) exchange.
*
* \param[in]   name    The name of the channel to retrieve.
* \retval      NULL    Channel does not exist.
*/
QAmqpChannel* QAmqpChannelHash::get(const QString& name) const
{
    if (name.isEmpty())
        return m_channels.value(QString());
    return m_channels.value(name);
}

QStringList QAmqpChannelHash::channels() const
{
  return m_channels.keys();
}

/*!
* Return true if the named channel exists.
*/
bool QAmqpChannelHash::contains(const QString& name) const
{
    if (name.isEmpty())
        return m_channels.contains(QString());
    return m_channels.contains(name);
}

/*!
* Store an exchange in the hash.  The nameless exchange is stored under
* the name "".
*/
void QAmqpChannelHash::put(QAmqpExchange* exchange)
{
    if (exchange->name().isEmpty())
        put(QString(), exchange);
    else
        put(exchange->name(), exchange);
}

/*!
* Store a queue in the hash.  If the queue is nameless, we hook its
* declared signal and store it when the queue receives a name from the
* broker, otherwise we store it under the name given.
*/
void QAmqpChannelHash::put(QAmqpQueue* queue)
{
    if (queue->name().isEmpty())
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
    else
        put(queue->name(), queue);
}

/*!
* Handle destruction of a channel.  Do a full garbage collection run.
*/
void QAmqpChannelHash::channelDestroyed(QObject* object)
{
    QList<QString> names(m_channels.keys());
    QList<QString>::iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        if (m_channels.value(*it) == object)
            m_channels.remove(*it);
    }
}

/*!
* Handle a queue that has just been declared and given a new name.  The
* caller is assumed to be a QAmqpQueue instance.
*/
void QAmqpChannelHash::queueDeclared()
{
    QAmqpQueue *queue = qobject_cast<QAmqpQueue*>(sender());
    if (queue)
        put(queue);
}

/*!
* Store a channel in the hash.  The channel is assumed
* to be named at the time of storage.  This hooks the 'destroyed' signal
* so the channel can be removed from our list.
*/
void QAmqpChannelHash::put(const QString& name, QAmqpChannel* channel)
{
    connect(channel, SIGNAL(destroyed(QObject*)), this, SLOT(channelDestroyed(QObject*)));
    m_channels[name] = channel;
}

/* vim: set ts=4 sw=4 et */
