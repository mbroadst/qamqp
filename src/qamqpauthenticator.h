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
#ifndef QAMQPAUTHENTICATOR_H
#define QAMQPAUTHENTICATOR_H

#include <QString>
#include <QDataStream>

#include "qamqpglobal.h"

class QAMQP_EXPORT QAmqpAuthenticator
{
public:
    virtual ~QAmqpAuthenticator() {}
    virtual QString type() const = 0;
    virtual void write(QDataStream &out) = 0;
};

class QAMQP_EXPORT QAmqpPlainAuthenticator : public QAmqpAuthenticator
{
public:
    QAmqpPlainAuthenticator(const QString &login = QString(), const QString &password = QString());
    virtual ~QAmqpPlainAuthenticator();

    QString login() const;
    void setLogin(const QString &l);

    QString password() const;
    void setPassword(const QString &p);

    virtual QString type() const;
    virtual void write(QDataStream &out);

private:
    QString login_;
    QString password_;

};

#endif // QAMQPAUTHENTICATOR_H
