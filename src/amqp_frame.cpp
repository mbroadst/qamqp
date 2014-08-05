#include "amqp_frame.h"

#include <QDateTime>
#include <QList>
#include <QDebug>

#include "amqp_table.h"
#include "amqp_global.h"

using namespace QAMQP;
using namespace QAMQP::Frame;

Base::Base(Type type)
    : size_(0),
      type_(type),
      channel_(0)
{
}

Base::Base(QDataStream &raw)
{
    readHeader(raw);
}

Type Base::type() const
{
    return Type(type_);
}

Base::~Base()
{
}

void Base::setChannel(qint16 channel)
{
    channel_ = channel;
}

qint16 Base::channel() const
{
    return channel_;
}

qint32 Base::size() const
{
    return 0;
}

void Base::writeHeader(QDataStream &stream) const
{
    stream << type_;
    stream << channel_;
    stream << qint32(size());
}

void Base::writeEnd(QDataStream &stream) const
{
    stream << qint8(FRAME_END);
    stream.device()->waitForBytesWritten(1000);
}

void Base::readHeader(QDataStream &stream)
{
    stream >> type_;
    stream >> channel_;
    stream >> size_;
}

/*
void Base::readEnd(QDataStream &stream)
{
    unsigned char end_  = 0;
    stream.readRawData(reinterpret_cast<char*>(&end_), sizeof(end_));
    if (end_ != FRAME_END)
        qWarning("Wrong end of frame");
}
*/
void Base::toStream(QDataStream &stream) const
{
    writeHeader(stream);
    writePayload(stream);
    writeEnd(stream);
}

//////////////////////////////////////////////////////////////////////////

Method::Method(MethodClass methodClass, qint16 id)
    : Base(ftMethod),
      methodClass_(methodClass),
      id_(id)
{
}

Method::Method(QDataStream &raw)
    : Base(raw)
{
    readPayload(raw);
}

MethodClass Method::methodClass() const
{
    return MethodClass(methodClass_);
}

qint16 Method::id() const
{
    return id_;
}

qint32 Method::size() const
{
    return sizeof(id_) + sizeof(methodClass_) + arguments_.size();
}

void Method::setArguments(const QByteArray &data)
{
    arguments_ = data;
}

QByteArray Method::arguments() const
{
    return arguments_;
}

void Method::readPayload(QDataStream &stream)
{
    stream >> methodClass_;
    stream >> id_;

    arguments_.resize(size_ - (sizeof(id_) + sizeof(methodClass_)));
    stream.readRawData(arguments_.data(), arguments_.size());
}

void Method::writePayload(QDataStream &stream) const
{
    stream << quint16(methodClass_);
    stream << quint16(id_);
    stream.writeRawData(arguments_.data(), arguments_.size());
}

//////////////////////////////////////////////////////////////////////////

QVariant Frame::readAmqpField(QDataStream &s, QAMQP::ValueType type)
{
    switch (type) {
    case Boolean:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<bool>(octet > 0);
    }
    case ShortShortUint:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<int>(octet);
    }
    case ShortUint:
    {
        quint16 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case LongUint:
    {
        quint32 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case LongLongUint:
    {
        qulonglong v = 0 ;
        s >> v;
        return v;
    }
    case ShortString:
    {
        qint8 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromLatin1(buffer.data(), size);
    }
    case LongString:
    {
        quint32 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromUtf8(buffer.data(), buffer.size());
    }
    case Timestamp:
    {
        qulonglong tmp_value;
        s >> tmp_value;
        return QDateTime::fromMSecsSinceEpoch(tmp_value);
    }
    case Hash:
    {
        Table table;
        s >> table;
        return table;
    }
    case Void:
        return QVariant();
    default:
        qWarning() << Q_FUNC_INFO << "unsupported value type: " << type;
    }

    return QVariant();
}

void Frame::writeAmqpField(QDataStream &s, QAMQP::ValueType type, const QVariant &value)
{
    switch (type) {
    case Boolean:
        s << (value.toBool() ? qint8(1) : qint8(0));
        break;
    case ShortShortUint:
        s << qint8(value.toUInt());
        break;
    case ShortUint:
        s << quint16(value.toUInt());
        break;
    case LongUint:
        s << quint32(value.toUInt());
        break;
    case LongLongUint:
        s << qulonglong(value.toULongLong());
        break;
    case ShortString:
    {
        QString str = value.toString();
        if (str.length() >= 256) {
            qAmqpDebug() << Q_FUNC_INFO << "invalid shortstr length: " << str.length();
        }

        s << quint8(str.length());
        s.writeRawData(str.toUtf8().data(), str.length());
    }
        break;
    case LongString:
    {
        QString str = value.toString();
        s << quint32(str.length());
        s.writeRawData(str.toLatin1().data(), str.length());
    }
        break;
    case Timestamp:
        s << qulonglong(value.toDateTime().toMSecsSinceEpoch());
        break;
    case Hash:
    {
        Table table(value.toHash());
        s << table;
    }
        break;
    default:
        qWarning() << Q_FUNC_INFO << "unsupported value type: " << type;
    }
}

//////////////////////////////////////////////////////////////////////////

Content::Content()
    : Base(ftHeader)
{
}

Content::Content(MethodClass methodClass)
    : Base(ftHeader)
{
    methodClass_ = methodClass;
}

Content::Content(QDataStream &raw)
    : Base(raw)
{
    readPayload(raw);
}

MethodClass Content::methodClass() const
{
    return MethodClass(methodClass_);
}

qint32 Content::size() const
{
    QDataStream out(&buffer_, QIODevice::WriteOnly);
    buffer_.clear();
    out << qint16(methodClass_);
    out << qint16(0); //weight
    out << qlonglong(bodySize_);

    qint16 prop_ = 0;
    foreach (int p, properties_.keys())
        prop_ |= p;
    out << prop_;

    if (prop_ & cpContentType)
        writeAmqpField(out, ShortString, properties_[cpContentType]);

    if (prop_ & cpContentEncoding)
        writeAmqpField(out, ShortString, properties_[cpContentEncoding]);

    if (prop_ & cpHeaders)
        writeAmqpField(out, Hash, properties_[cpHeaders]);

    if (prop_ & cpDeliveryMode)
        writeAmqpField(out, ShortShortUint, properties_[cpDeliveryMode]);

    if (prop_ & cpPriority)
        writeAmqpField(out, ShortShortUint, properties_[cpPriority]);

    if (prop_ & cpCorrelationId)
        writeAmqpField(out, ShortString, properties_[cpCorrelationId]);

    if (prop_ & cpReplyTo)
        writeAmqpField(out, ShortString, properties_[cpReplyTo]);

    if (prop_ & cpExpiration)
        writeAmqpField(out, ShortString, properties_[cpExpiration]);

    if (prop_ & cpMessageId)
        writeAmqpField(out, ShortString, properties_[cpMessageId]);

    if (prop_ & cpTimestamp)
        writeAmqpField(out, Timestamp, properties_[cpTimestamp]);

    if (prop_ & cpType)
        writeAmqpField(out, ShortString, properties_[cpType]);

    if (prop_ & cpUserId)
        writeAmqpField(out, ShortString, properties_[cpUserId]);

    if (prop_ & cpAppId)
        writeAmqpField(out, ShortString, properties_[cpAppId]);

    if (prop_ & cpClusterID)
        writeAmqpField(out, ShortString, properties_[cpClusterID]);

    return buffer_.size();
}

qlonglong Content::bodySize() const
{
    return bodySize_;
}

void Content::setBodySize(qlonglong size)
{
    bodySize_ = size;
}

void Content::setProperty(Property prop, const QVariant &value)
{
    properties_[prop] = value;
}

QVariant Content::property(Property prop) const
{
    return properties_.value(prop);
}

void Content::writePayload(QDataStream &out) const
{
    out.writeRawData(buffer_.data(), buffer_.size());
}

void Content::readPayload(QDataStream &in)
{
    in >> methodClass_;
    in.skipRawData(2); //weight
    in >> bodySize_;
    qint16 flags_ = 0;
    in >> flags_;
    if (flags_ & cpContentType)
        properties_[cpContentType] = readAmqpField(in, ShortString);

    if (flags_ & cpContentEncoding)
        properties_[cpContentEncoding] = readAmqpField(in, ShortString);

    if (flags_ & cpHeaders)
        properties_[cpHeaders] = readAmqpField(in, Hash);

    if (flags_ & cpDeliveryMode)
        properties_[cpDeliveryMode] = readAmqpField(in, ShortShortUint);

    if (flags_ & cpPriority)
        properties_[cpPriority] = readAmqpField(in, ShortShortUint);

    if (flags_ & cpCorrelationId)
        properties_[cpCorrelationId] = readAmqpField(in, ShortString);

    if (flags_ & cpReplyTo)
        properties_[cpReplyTo] = readAmqpField(in, ShortString);

    if (flags_ & cpExpiration)
        properties_[cpExpiration] = readAmqpField(in, ShortString);

    if (flags_ & cpMessageId)
        properties_[cpMessageId] = readAmqpField(in, ShortString);

    if (flags_ & cpTimestamp)
        properties_[cpTimestamp] = readAmqpField(in, Timestamp);

    if (flags_ & cpType)
        properties_[cpType] = readAmqpField(in, ShortString);

    if (flags_ & cpUserId)
        properties_[cpUserId] = readAmqpField(in, ShortString);

    if (flags_ & cpAppId)
        properties_[cpAppId] = readAmqpField(in, ShortString);

    if (flags_ & cpClusterID)
        properties_[cpClusterID] = readAmqpField(in, ShortString);
}

//////////////////////////////////////////////////////////////////////////

ContentBody::ContentBody()
    : Base(ftBody)
{
}

ContentBody::ContentBody(QDataStream &raw)
    : Base(raw)
{
    readPayload(raw);
}

void ContentBody::setBody(const QByteArray &data)
{
    body_ = data;
}

QByteArray ContentBody::body() const
{
    return body_;
}

void ContentBody::writePayload(QDataStream &out) const
{
    out.writeRawData(body_.data(), body_.size());
}

void ContentBody::readPayload(QDataStream &in)
{
    body_.resize(size_);
    in.readRawData(body_.data(), body_.size());
}

qint32 ContentBody::size() const
{
    return body_.size();
}

//////////////////////////////////////////////////////////////////////////

Heartbeat::Heartbeat()
    : Base(ftHeartbeat)
{
}

void Heartbeat::readPayload(QDataStream &stream)
{
    Q_UNUSED(stream)
}

void Heartbeat::writePayload(QDataStream &stream) const
{
    Q_UNUSED(stream)
}
