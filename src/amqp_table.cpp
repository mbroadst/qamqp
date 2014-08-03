#include <float.h>

#include <QDateTime>
#include <QDebug>

#include "amqp_frame.h"
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

ValueType valueTypeForOctet(qint8 octet)
{
    switch (octet) {
    case 't': return Boolean;
    case 'b': return ShortShortInt;
    case 's': return ShortInt;
    case 'I': return LongInt;
    case 'l': return LongLongInt;
    case 'f': return Float;
    case 'd': return Double;
    case 'D': return Decimal;
    case 'S': return LongString;
    case 'A': return Array;
    case 'T': return Timestamp;
    case 'F': return Hash;
    case 'V': return Void;
    case 'x': return Bytes;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid octet received: " << char(octet);
    }

    return Invalid;
}

qint8 valueTypeToOctet(ValueType type)
{
    switch (type) {
    case Boolean: return 't';
    case ShortShortInt: return 'b';
    case ShortInt: return 's';
    case LongInt: return 'I';
    case LongLongInt: return 'l';
    case Float: return 'f';
    case Double: return 'd';
    case Decimal: return 'D';
    case LongString: return 'S';
    case Array: return 'A';
    case Timestamp: return 'T';
    case Hash: return 'F';
    case Void: return 'V';
    case Bytes: return 'x';
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid type received: " << char(type);
    }

    return 'V';
}

void Table::writeFieldValue(QDataStream &stream, const QVariant &value)
{
    ValueType type;
    switch (value.userType()) {
    case QMetaType::Bool:
        type = Boolean;
        break;
    case QMetaType::QByteArray:
        type = Bytes;
        break;
    case QMetaType::Int:
    {
        int i = qAbs(value.toInt());
        if (i <= qint8(UINT8_MAX)) {
            type = ShortShortInt;
        } else if (i <= qint16(UINT16_MAX)) {
            type = ShortInt;
        } else {
            type = LongInt;
        }
    }
        break;
    case QMetaType::UShort:
        type = ShortInt;
        break;
    case QMetaType::UInt:
    {
        int i = value.toInt();
        if (i <= qint8(UINT8_MAX)) {
            type = ShortShortInt;
        } else if (i <= qint16(UINT16_MAX)) {
            type = ShortInt;
        } else {
            type = LongInt;
        }
    }
        break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        type = LongLongInt;
        break;
    case QMetaType::QString:
        type = LongString;
        break;
    case QMetaType::QDateTime:
        type = Timestamp;
        break;
    case QMetaType::Double:
        type = value.toDouble() > FLT_MAX ? Double : Float;
        break;
    case QMetaType::QVariantHash:
        type = Hash;
        break;
    case QMetaType::QVariantList:
        type = Array;
        break;
    case QMetaType::Void:
        type = Void;
        break;
    default:
        if (value.userType() == qMetaTypeId<Frame::decimal>()) {
            type = Decimal;
            break;
        } else if (!value.isValid()) {
            type = Void;
            break;
        }

        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << value.userType();
        return;
    }

    // write the field value type, a requirement for field tables only
    stream << valueTypeToOctet(type);
    writeFieldValue(stream, type, value);
}

void Table::writeFieldValue(QDataStream &stream, ValueType type, const QVariant &value)
{
    switch (type) {
    case Boolean:
    case ShortShortUint:
    case ShortUint:
    case LongUint:
    case LongLongUint:
    case ShortString:
    case LongString:
    case Timestamp:
    case Hash:
        return Frame::writeAmqpField(stream, type, value);

    case ShortShortInt:
        stream << qint8(value.toInt());
        break;
    case ShortInt:
        stream << qint16(value.toInt());
        break;
    case LongInt:
        stream << qint32(value.toInt());
        break;
    case LongLongInt:
        stream << qlonglong(value.toLongLong());
        break;
    case Float:
    {
        float g = value.toFloat();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case Double:
    {
        double g = value.toDouble();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case Decimal:
    {
        Frame::decimal v(value.value<Frame::decimal>());
        stream << v.scale;
        stream << v.value;
    }
        break;
    case Array:
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
    case Bytes:
    {
        QByteArray ba = value.toByteArray();
        stream << quint32(ba.length());
        stream.writeRawData(ba.data(), ba.length());
    }
        break;
    case Void:
        stream << qint32(0);
        break;

    default:
        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << type;
    }
}

QVariant Table::readFieldValue(QDataStream &stream, ValueType type)
{
    switch (type) {
    case Boolean:
    case ShortShortUint:
    case ShortUint:
    case LongUint:
    case LongLongUint:
    case ShortString:
    case LongString:
    case Timestamp:
    case Hash:
        return Frame::readAmqpField(stream, type);

    case ShortShortInt:
    {
        char octet;
        stream.readRawData(&octet, sizeof(octet));
        return QVariant::fromValue<int>(octet);
    }
    case ShortInt:
    {
        qint16 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case LongInt:
    {
        qint32 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case LongLongInt:
    {
        qlonglong v = 0 ;
        stream >> v;
        return v;
    }
    case Float:
    {
        float tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<float>(tmp_value);
    }
    case Double:
    {
        double tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<double>(tmp_value);
    }
    case Decimal:
    {
        Frame::decimal v;
        stream >> v.scale;
        stream >> v.value;
        return QVariant::fromValue<Frame::decimal>(v);
    }
    case Array:
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
    case Bytes:
    {
        QByteArray bytes;
        quint32 length = 0;
        stream >> length;
        bytes.resize(length);
        stream.readRawData(bytes.data(), bytes.size());
        return bytes;
    }
    case Void:
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
        Table::writeFieldValue(s, ShortString, it.key());
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
        QString field = Frame::readAmqpField(tableStream, ShortString).toString();
        tableStream >> octet;
        table[field] = Table::readFieldValue(tableStream, valueTypeForOctet(octet));
    }

    return stream;
}
