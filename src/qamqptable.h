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
#ifndef QAMQPTABLE_H
#define QAMQPTABLE_H

#include <QVariantHash>

#include "qamqpglobal.h"

class QAMQP_EXPORT QAmqpTable : public QVariantHash
{
public:
    QAmqpTable() {}
    inline QAmqpTable(const QVariantHash &variantHash)
        : QVariantHash(variantHash)
    {
    }

    static void writeFieldValue(QDataStream &stream, const QVariant &value);
    static void writeFieldValue(QDataStream &stream, QAmqpMetaType::ValueType type, const QVariant &value);
    static QVariant readFieldValue(QDataStream &stream, QAmqpMetaType::ValueType type);
};

QAMQP_EXPORT QDataStream &operator<<(QDataStream &, const QAmqpTable &table);
QAMQP_EXPORT QDataStream &operator>>(QDataStream &, QAmqpTable &table);
Q_DECLARE_METATYPE(QAmqpTable)

#endif  // QAMQPTABLE_H
