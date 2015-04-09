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
    if (name.isNull())
        return channels.value(QString(""));
    return channels.value(name);
}

/*!
* Store an exchange in the hash.  The nameless exchange is stored under
* the name "".
*/
void QAmqpChannelHash::put(QAmqpExchange* exchange)
{
    if (exchange->name().isEmpty())
        put(QString(""), exchange);
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
        connect(queue,  SIGNAL(declared()),
                this,   SLOT(queueDeclared()));
    else
        put(queue->name(), queue);
}

/*!
* Handle destruction of a channel.  Retrieve its current name and
* remove it from the list, otherwise do a full garbage collection
* run.
*/
void QAmqpChannelHash::channelDestroyed()
{
    QAmqpChannel* ch = static_cast<QAmqpChannel*>(sender());
    if (!channels.contains(ch->name()))
        return;
    if (channels[ch->name()] == ch)
        channels.remove(ch->name());
}

/*!
* Handle a queue that has just been declared and given a new name.  The
* caller is assumed to be a QAmqpQueue instance.
*/
void QAmqpChannelHash::queueDeclared()
{
    put(static_cast<QAmqpQueue*>(sender()));
}

/*!
* Store a channel in the hash.  The channel is assumed
* to be named at the time of storage.  This hooks the 'destroyed' signal
* so the channel can be removed from our list.
*/
void QAmqpChannelHash::put(const QString& name, QAmqpChannel* channel)
{
    connect(channel,    SIGNAL(destroyed()),
            this,       SLOT(channelDestroyed()));
    channels[name] = channel;
}

#include "moc_qamqpchannelhash_p.cpp"
/* vim: set ts=4 sw=4 et */
