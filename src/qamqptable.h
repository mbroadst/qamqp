#ifndef QAMQPTABLE_H
#define QAMQPTABLE_H

#include <QVariantHash>

#include "qamqpglobal.h"

namespace QAMQP {

class QAMQP_EXPORT Table : public QVariantHash
{
public:
    Table() {}
    inline Table(const QVariantHash &variantHash)
        : QVariantHash(variantHash)
    {
    }

    static void writeFieldValue(QDataStream &stream, const QVariant &value);
    static void writeFieldValue(QDataStream &stream, MetaType::ValueType type, const QVariant &value);
    static QVariant readFieldValue(QDataStream &stream, MetaType::ValueType type);
};

}   // namespace QAMQP

QAMQP_EXPORT QDataStream &operator<<(QDataStream &, const QAMQP::Table &table);
QAMQP_EXPORT QDataStream &operator>>(QDataStream &, QAMQP::Table &table);
Q_DECLARE_METATYPE(QAMQP::Table)

#endif  // QAMQPTABLE_H
