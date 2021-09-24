#include <float.h>

#include <QDateTime>
#include <QDebug>

#include "qamqpframe_p.h"
#include "qamqptable.h"

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

QAmqpMetaType::ValueType valueTypeForOctet(qint8 octet)
{
    switch (octet) {
    case 't': return QAmqpMetaType::Boolean;
    case 'b': return QAmqpMetaType::ShortShortInt;
    case 's': return QAmqpMetaType::ShortInt;
    case 'I': return QAmqpMetaType::LongInt;
    case 'l': return QAmqpMetaType::LongLongInt;
    case 'f': return QAmqpMetaType::Float;
    case 'd': return QAmqpMetaType::Double;
    case 'D': return QAmqpMetaType::Decimal;
    case 'S': return QAmqpMetaType::LongString;
    case 'A': return QAmqpMetaType::Array;
    case 'T': return QAmqpMetaType::Timestamp;
    case 'F': return QAmqpMetaType::Hash;
    case 'V': return QAmqpMetaType::Void;
    case 'x': return QAmqpMetaType::Bytes;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid octet received: " << char(octet);
    }

    return QAmqpMetaType::Invalid;
}

qint8 valueTypeToOctet(QAmqpMetaType::ValueType type)
{
    switch (type) {
    case QAmqpMetaType::Boolean: return 't';
    case QAmqpMetaType::ShortShortInt: return 'b';
    case QAmqpMetaType::ShortInt: return 's';
    case QAmqpMetaType::LongInt: return 'I';
    case QAmqpMetaType::LongLongInt: return 'l';
    case QAmqpMetaType::Float: return 'f';
    case QAmqpMetaType::Double: return 'd';
    case QAmqpMetaType::Decimal: return 'D';
    case QAmqpMetaType::LongString: return 'S';
    case QAmqpMetaType::Array: return 'A';
    case QAmqpMetaType::Timestamp: return 'T';
    case QAmqpMetaType::Hash: return 'F';
    case QAmqpMetaType::Void: return 'V';
    case QAmqpMetaType::Bytes: return 'x';
    default:
        qAmqpDebug() << Q_FUNC_INFO << "invalid type received: " << char(type);
    }

    return 'V';
}

void QAmqpTable::writeFieldValue(QDataStream &stream, const QVariant &value)
{
    QAmqpMetaType::ValueType type;
    switch (value.userType()) {
    case QMetaType::Bool:
        type = QAmqpMetaType::Boolean;
        break;
    case QMetaType::QByteArray:
        type = QAmqpMetaType::Bytes;
        break;
    case QMetaType::Int:
    {
        int i = qAbs(value.toInt());
        if (i <= qint8(SCHAR_MAX)) {
            type = QAmqpMetaType::ShortShortInt;
        } else if (i <= qint16(SHRT_MAX)) {
            type = QAmqpMetaType::ShortInt;
        } else {
            type = QAmqpMetaType::LongInt;
        }
    }
        break;
    case QMetaType::UShort:
        type = QAmqpMetaType::ShortInt;
        break;
    case QMetaType::UInt:
    {
        int i = value.toInt();
        if (i <= qint8(SCHAR_MAX)) {
            type = QAmqpMetaType::ShortShortInt;
        } else if (i <= qint16(SHRT_MAX)) {
            type = QAmqpMetaType::ShortInt;
        } else {
            type = QAmqpMetaType::LongInt;
        }
    }
        break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        type = QAmqpMetaType::LongLongInt;
        break;
    case QMetaType::QString:
        type = QAmqpMetaType::LongString;
        break;
    case QMetaType::QDateTime:
        type = QAmqpMetaType::Timestamp;
        break;
    case QMetaType::Double:
        type = value.toDouble() > FLT_MAX ? QAmqpMetaType::Double : QAmqpMetaType::Float;
        break;
    case QMetaType::QVariantHash:
        type = QAmqpMetaType::Hash;
        break;
    case QMetaType::QVariantList:
        type = QAmqpMetaType::Array;
        break;
    case QMetaType::Void:
        type = QAmqpMetaType::Void;
        break;
    default:
        if (value.userType() == qMetaTypeId<QAMQP::Decimal>()) {
            type = QAmqpMetaType::Decimal;
            break;
        } else if (!value.isValid()) {
            type = QAmqpMetaType::Void;
            break;
        }

        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << value.userType();
        return;
    }

    // write the field value type, a requirement for field tables only
    stream << valueTypeToOctet(type);
    writeFieldValue(stream, type, value);
}

void QAmqpTable::writeFieldValue(QDataStream &stream, QAmqpMetaType::ValueType type, const QVariant &value)
{
    switch (type) {
    case QAmqpMetaType::Boolean:
    case QAmqpMetaType::ShortShortUint:
    case QAmqpMetaType::ShortUint:
    case QAmqpMetaType::LongUint:
    case QAmqpMetaType::LongLongUint:
    case QAmqpMetaType::ShortString:
    case QAmqpMetaType::LongString:
    case QAmqpMetaType::Timestamp:
    case QAmqpMetaType::Hash:
        return QAmqpFrame::writeAmqpField(stream, type, value);

    case QAmqpMetaType::ShortShortInt:
        stream << qint8(value.toInt());
        break;
    case QAmqpMetaType::ShortInt:
        stream << qint16(value.toInt());
        break;
    case QAmqpMetaType::LongInt:
        stream << qint32(value.toInt());
        break;
    case QAmqpMetaType::LongLongInt:
        stream << qlonglong(value.toLongLong());
        break;
    case QAmqpMetaType::Float:
    {
        float g = value.toFloat();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case QAmqpMetaType::Double:
    {
        double g = value.toDouble();
        QDataStream::FloatingPointPrecision oldPrecision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream << g;
        stream.setFloatingPointPrecision(oldPrecision);
    }
        break;
    case QAmqpMetaType::Decimal:
    {
        QAMQP::Decimal v(value.value<QAMQP::Decimal>());
        stream << v.scale;
        stream << v.value;
    }
        break;
    case QAmqpMetaType::Array:
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
    case QAmqpMetaType::Bytes:
    {
        QByteArray ba = value.toByteArray();
        stream << quint32(ba.length());
        stream.writeRawData(ba.data(), ba.length());
    }
        break;
    case QAmqpMetaType::Void:
        stream << qint32(0);
        break;

    default:
        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << type;
    }
}

QVariant QAmqpTable::readFieldValue(QDataStream &stream, QAmqpMetaType::ValueType type)
{
    switch (type) {
    case QAmqpMetaType::Boolean:
    case QAmqpMetaType::ShortShortUint:
    case QAmqpMetaType::ShortUint:
    case QAmqpMetaType::LongUint:
    case QAmqpMetaType::LongLongUint:
    case QAmqpMetaType::ShortString:
    case QAmqpMetaType::LongString:
    case QAmqpMetaType::Timestamp:
    case QAmqpMetaType::Hash:
        return QAmqpFrame::readAmqpField(stream, type);

    case QAmqpMetaType::ShortShortInt:
    {
        char octet;
        stream.readRawData(&octet, sizeof(octet));
        return QVariant::fromValue<int>(octet);
    }
    case QAmqpMetaType::ShortInt:
    {
        qint16 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case QAmqpMetaType::LongInt:
    {
        qint32 tmp_value = 0;
        stream >> tmp_value;
        return QVariant::fromValue<int>(tmp_value);
    }
    case QAmqpMetaType::LongLongInt:
    {
        qlonglong v = 0 ;
        stream >> v;
        return v;
    }
    case QAmqpMetaType::Float:
    {
        float tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<float>(tmp_value);
    }
    case QAmqpMetaType::Double:
    {
        double tmp_value;
        QDataStream::FloatingPointPrecision precision = stream.floatingPointPrecision();
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream >> tmp_value;
        stream.setFloatingPointPrecision(precision);
        return QVariant::fromValue<double>(tmp_value);
    }
    case QAmqpMetaType::Decimal:
    {
        QAMQP::Decimal v;
        stream >> v.scale;
        stream >> v.value;
        return QVariant::fromValue<QAMQP::Decimal>(v);
    }
    case QAmqpMetaType::Array:
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
    case QAmqpMetaType::Bytes:
    {
        QByteArray bytes;
        quint32 length = 0;
        stream >> length;
        bytes.resize(length);
        stream.readRawData(bytes.data(), bytes.size());
        return bytes;
    }
    case QAmqpMetaType::Void:
        break;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unhandled type: " << type;
    }

    return QVariant();
}

QDataStream &operator<<(QDataStream &stream, const QAmqpTable &table)
{
    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    QAmqpTable::ConstIterator it;
    QAmqpTable::ConstIterator itEnd = table.constEnd();
    for (it = table.constBegin(); it != itEnd; ++it) {
        QAmqpTable::writeFieldValue(s, QAmqpMetaType::ShortString, it.key());
        QAmqpTable::writeFieldValue(s, it.value());
    }

    if (data.isEmpty()) {
        stream << qint32(0);
    } else {
        stream << data;
    }

    return stream;
}

QDataStream &operator>>(QDataStream &stream, QAmqpTable &table)
{
    QByteArray data;
    stream >> data;
    QDataStream tableStream(&data, QIODevice::ReadOnly);
    while (!tableStream.atEnd()) {
        qint8 octet = 0;
        QString field = QAmqpFrame::readAmqpField(tableStream, QAmqpMetaType::ShortString).toString();
        tableStream >> octet;
        table.insert(field, QAmqpTable::readFieldValue(tableStream, valueTypeForOctet(octet)));
    }

    return stream;
}
