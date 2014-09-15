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
