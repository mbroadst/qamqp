#include <float.h>

#include <QDateTime>
#include <QDebug>

#include "amqp_frame_p.h"
#include "amqp_table.h"
using namespace QAMQP;

/*
 * field value types according to: https://www.rabbitmq.com/amqp-0-9-1-errata.html
t - Boolean
b - Signed 8-bit
    Unsigned 8-bit
s - Signed 16-bit
    Unsigned 16-bit
I - Signed 32-bit
    Unsigned 32-bit
l - Signed 64-bit
    Unsigned 64-bit
f - 32-bit float
d - 64-bit float
D - Decimal
S - Long string
A - Array
T - Timestamp (u64)
F - Nested Table
V - Void
x - Byte array
*/

MetaType::ValueType valueTypeForOctet(qint8 octet)
{
    switch (octet) {
    case 't': return MetaType::Boolean;
    case 'b': return MetaType::ShortShortInt;
    case 's': return MetaType::ShortInt;
    case 'I': return MetaType::LongInt;
    case 'l': return MetaType::LongLongInt;
    case 'f': return MetaType::Float;
    case 'd': return MetaType::Double;
    case 'D': return MetaType::Decimal;
    case 'S': return MetaType::LongString;
    case 'A': return MetaType::Array;
    case 'T': return MetaType::Timestamp;
    case 'F': return MetaType::Hash;
    case 'V': return MetaType::Void;
    case 'x': return MetaType::Bytes;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid octet received: " << char(octet);
    }

    return MetaType::Invalid;
}

qint8 valueTypeToOctet(MetaType::ValueType type)
{
    switch (type) {
    case MetaType::Boolean: return 't';
    case MetaType::ShortShortInt: return 'b';
    case MetaType::ShortInt: return 's';
    case MetaType::LongInt: return 'I';
    case MetaType::LongLongInt: return 'l';
    case MetaType::Float: return 'f';
    case MetaType::Double: return 'd';
    case MetaType::Decimal: return 'D';
    case MetaType::LongString: return 'S';
    case MetaType::Array: return 'A';
    case MetaType::Timestamp: return 'T';
    case MetaType::Hash: return 'F';
    case MetaType::Void: return 'V';
    case MetaType::Bytes: return 'x';
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid type received: " << char(type);
    }

    return 'V';
}

void Table::writeFieldValue(QDataStream &stream, const QVariant &value)
{
    MetaType::ValueType type;
    switch (value.userType()) {
    case QMetaType::Bool:
        type = MetaType::Boolean;
        break;
    case QMetaType::QByteArray:
        type = MetaType::Bytes;
        break;
    case QMetaType::Int:
    {
        int i = qAbs(value.toInt());
        if (i <= qint8(SCHAR_MAX)) {
            type = MetaType::ShortShortInt;
        } else if (i <= qint16(SHRT_MAX)) {
            type = MetaType::ShortInt;
        } else {
            type = MetaType::LongInt;
        }
    }
        break;
    case QMetaType::UShort:
        type = MetaType::ShortInt;
        break;
    case QMetaType::UInt:
    {
        int i = value.toInt();
        if (i <= qint8(SCHAR_MAX)) {
            type = MetaType::ShortShortInt;
        } else if (i <= qint16(SHRT_MAX)) {
            type = MetaType::ShortInt;
        } else {
            type = MetaType::LongInt;
        }
    }
        break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        type = MetaType::LongLongInt;
        break;
    case QMetaType::QString:
        type = MetaType::LongString;
        break;
    case QMetaType::QDateTime:
        type = MetaType::Timestamp;
        break;
    case QMetaType::Double:
        type = value.toDouble() > FLT_MAX ? MetaType::Double : MetaType::Float;
        break;
    case QMetaType::QVariantHash:
        type = MetaType::Hash;
        break;
    case QMetaType::QVariantList:
        type = MetaType::Array;
        break;
    case QMetaType::Void:
        type = MetaType::Void;
        break;
    default:
        if (value.userType() == qMetaTypeId<QAMQP::Decimal>()) {
            type = MetaType::Decimal;
            break;
        } else if (!value.isValid()) {
            type = MetaType::Void;
            break;
        }

        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << value.userType();
        return;
    }

    // write the field value type, a requirement for field tables only
    stream << valueTypeToOctet(type);
    writeFieldValue(stream, type, value);
}

void Table::writeFieldValue(QDataStream &stream, MetaType::ValueType type, const QVariant &value)
{
    switch (type) {
    case MetaType::Boolean:
    case MetaType::ShortShortUint:
    case MetaType::ShortUint:
    case MetaType::LongUint:
    case MetaType::LongLongUint:
    case MetaType::ShortString:
    case MetaType::LongString:
    case MetaType::Timestamp:
    case MetaType::Hash:
        return Frame::writeAmqpField(stream, type, value);

    case MetaType::ShortShortInt:
        stream << qint8(value.toInt());
        break;
    case MetaType::ShortInt:
        stream << qint16(value.toInt());
        break;
    case MetaType::LongInt:
        stream << qint32(value.toInt());
        break;
    case MetaType::LongLongInt:
        stream << qlonglong(value.toLongLong());
        break;
    case MetaType::Float:
    {
        float g = value.toFloat();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case MetaType::Double:
    {
        double g = value.toDouble();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case MetaType::Decimal:
    {
        QAMQP::Decimal v(value.value<QAMQP::Decimal>());
        stream << v.scale;
        stream << v.value;
    }
        break;
    case MetaType::Array:
    {
        QByteArray buffer;
        QDataStream arrayStream(&buffer, QIODevice::WriteOnly);
        QVariantList array(value.toList());
        for (int i = 0; i < array.size(); ++i)
            writeFieldValue(arrayStream, array.at(i));

        if (buffer.isEmpty()) {
            stream << qint32(0);
        } else {
            stream << buffer;
        }
    }
        break;
    case MetaType::Bytes:
    {
        QByteArray ba = value.toByteArray();
        stream << quint32(ba.length());
        stream.writeRawData(ba.data(), ba.length());
    }
        break;
    case MetaType::Void:
        stream << qint32(0);
        break;

    default:
        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << type;
    }
}

QVariant Table::readFieldValue(QDataStream &stream, MetaType::ValueType type)
{
    switch (type) {
    case MetaType::Boolean:
    case MetaType::ShortShortUint:
    case MetaType::ShortUint:
    case MetaType::LongUint:
    case MetaType::LongLongUint:
    case MetaType::ShortString:
    case MetaType::LongString:
    case MetaType::Timestamp:
    case MetaType::Hash:
        return Frame::readAmqpField(stream, type);

    case MetaType::ShortShortInt:
    {
        char octet;
        stream.readRawData(&octet, sizeof(octet));
        return QVariant::fromValue<int>(octet);
    }
    case MetaType::ShortInt:
    {
        qint16 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case MetaType::LongInt:
    {
        qint32 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case MetaType::LongLongInt:
    {
        qlonglong v = 0 ;
        stream >> v;
        return v;
    }
    case MetaType::Float:
    {
        float tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<float>(tmp_value);
    }
    case MetaType::Double:
    {
        double tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<double>(tmp_value);
    }
    case MetaType::Decimal:
    {
        QAMQP::Decimal v;
        stream >> v.scale;
        stream >> v.value;
        return QVariant::fromValue<QAMQP::Decimal>(v);
    }
    case MetaType::Array:
    {
        QByteArray data;
        quint32 size = 0;
        stream >> size;
        data.resize(size);
        stream.readRawData(data.data(), data.size());

        qint8 type = 0;
        QVariantList result;
        QDataStream arrayStream(&data, QIODevice::ReadOnly);
        while (!arrayStream.atEnd()) {
            arrayStream >> type;
            result.append(readFieldValue(arrayStream, valueTypeForOctet(type)));
        }

        return result;
    }
    case MetaType::Bytes:
    {
        QByteArray bytes;
        quint32 length = 0;
        stream >> length;
        bytes.resize(length);
        stream.readRawData(bytes.data(), bytes.size());
        return bytes;
    }
    case MetaType::Void:
        break;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << type;
    }

    return QVariant();
}

QDataStream &operator<<(QDataStream &stream, const Table &table)
{
    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    Table::ConstIterator it;
    Table::ConstIterator itEnd = table.constEnd();
    for (it = table.constBegin(); it != itEnd; ++it) {
        Table::writeFieldValue(s, MetaType::ShortString, it.key());
        Table::writeFieldValue(s, it.value());
    }

    if (data.isEmpty()) {
        stream << qint32(0);
    } else {
        stream << data;
    }

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Table &table)
{
    QByteArray data;
    stream >> data;
    QDataStream tableStream(&data, QIODevice::ReadOnly);
    while (!tableStream.atEnd()) {
        qint8 octet = 0;
        QString field = Frame::readAmqpField(tableStream, MetaType::ShortString).toString();
        tableStream >> octet;
        table[field] = Table::readFieldValue(tableStream, valueTypeForOctet(octet));
    }

    return stream;
}
