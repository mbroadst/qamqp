#ifndef amqp_table_h__
#define amqp_table_h__

#include "amqp_global.h"
#include <QVariantHash>

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
    static void writeFieldValue(QDataStream &stream, ValueType type, const QVariant &value);
    static QVariant readFieldValue(QDataStream &stream, ValueType type);
};

}   // namespace QAMQP

QAMQP_EXPORT QDataStream &operator<<(QDataStream &, const QAMQP::Table &table);
QAMQP_EXPORT QDataStream &operator>>(QDataStream &, QAMQP::Table &table);
Q_DECLARE_METATYPE(QAMQP::Table)

#endif  // amqp_table_h__
