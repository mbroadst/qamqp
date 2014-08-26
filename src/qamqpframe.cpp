#include <QDateTime>
#include <QList>
#include <QDebug>

#include "qamqptable.h"
#include "qamqpglobal.h"
#include "qamqpframe_p.h"

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
        qAmqpDebug("Wrong end of frame");
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

QVariant Frame::readAmqpField(QDataStream &s, MetaType::ValueType type)
{
    switch (type) {
    case MetaType::Boolean:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<bool>(octet > 0);
    }
    case MetaType::ShortShortUint:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<int>(octet);
    }
    case MetaType::ShortUint:
    {
        quint16 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case MetaType::LongUint:
    {
        quint32 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case MetaType::LongLongUint:
    {
        qulonglong v = 0 ;
        s >> v;
        return v;
    }
    case MetaType::ShortString:
    {
        qint8 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromLatin1(buffer.data(), size);
    }
    case MetaType::LongString:
    {
        quint32 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromUtf8(buffer.data(), buffer.size());
    }
    case MetaType::Timestamp:
    {
        qulonglong tmp_value;
        s >> tmp_value;
        return QDateTime::fromMSecsSinceEpoch(tmp_value);
    }
    case MetaType::Hash:
    {
        Table table;
        s >> table;
        return table;
    }
    case MetaType::Void:
        return QVariant();
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unsupported value type: " << type;
    }

    return QVariant();
}

void Frame::writeAmqpField(QDataStream &s, MetaType::ValueType type, const QVariant &value)
{
    switch (type) {
    case MetaType::Boolean:
        s << (value.toBool() ? qint8(1) : qint8(0));
        break;
    case MetaType::ShortShortUint:
        s << qint8(value.toUInt());
        break;
    case MetaType::ShortUint:
        s << quint16(value.toUInt());
        break;
    case MetaType::LongUint:
        s << quint32(value.toUInt());
        break;
    case MetaType::LongLongUint:
        s << qulonglong(value.toULongLong());
        break;
    case MetaType::ShortString:
    {
        QString str = value.toString();
        if (str.length() >= 256) {
            qAmqpDebug() << Q_FUNC_INFO << "invalid shortstr length: " << str.length();
        }

        s << quint8(str.length());
        s.writeRawData(str.toUtf8().data(), str.length());
    }
        break;
    case MetaType::LongString:
    {
        QString str = value.toString();
        s << quint32(str.length());
        s.writeRawData(str.toLatin1().data(), str.length());
    }
        break;
    case MetaType::Timestamp:
        s << qulonglong(value.toDateTime().toMSecsSinceEpoch());
        break;
    case MetaType::Hash:
    {
        Table table(value.toHash());
        s << table;
    }
        break;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unsupported value type: " << type;
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

    if (prop_ & Message::ContentType)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::ContentType]);

    if (prop_ & Message::ContentEncoding)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::ContentEncoding]);

    if (prop_ & Message::Headers)
        writeAmqpField(out, MetaType::Hash, properties_[Message::Headers]);

    if (prop_ & Message::DeliveryMode)
        writeAmqpField(out, MetaType::ShortShortUint, properties_[Message::DeliveryMode]);

    if (prop_ & Message::Priority)
        writeAmqpField(out, MetaType::ShortShortUint, properties_[Message::Priority]);

    if (prop_ & Message::CorrelationId)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::CorrelationId]);

    if (prop_ & Message::ReplyTo)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::ReplyTo]);

    if (prop_ & Message::Expiration)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::Expiration]);

    if (prop_ & Message::MessageId)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::MessageId]);

    if (prop_ & Message::Timestamp)
        writeAmqpField(out, MetaType::Timestamp, properties_[Message::Timestamp]);

    if (prop_ & Message::Type)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::Type]);

    if (prop_ & Message::UserId)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::UserId]);

    if (prop_ & Message::AppId)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::AppId]);

    if (prop_ & Message::ClusterID)
        writeAmqpField(out, MetaType::ShortString, properties_[Message::ClusterID]);

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

void Content::setProperty(Message::Property prop, const QVariant &value)
{
    properties_[prop] = value;
}

QVariant Content::property(Message::Property prop) const
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
    if (flags_ & Message::ContentType)
        properties_[Message::ContentType] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::ContentEncoding)
        properties_[Message::ContentEncoding] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::Headers)
        properties_[Message::Headers] = readAmqpField(in, MetaType::Hash);

    if (flags_ & Message::DeliveryMode)
        properties_[Message::DeliveryMode] = readAmqpField(in, MetaType::ShortShortUint);

    if (flags_ & Message::Priority)
        properties_[Message::Priority] = readAmqpField(in, MetaType::ShortShortUint);

    if (flags_ & Message::CorrelationId)
        properties_[Message::CorrelationId] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::ReplyTo)
        properties_[Message::ReplyTo] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::Expiration)
        properties_[Message::Expiration] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::MessageId)
        properties_[Message::MessageId] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::Timestamp)
        properties_[Message::Timestamp] = readAmqpField(in, MetaType::Timestamp);

    if (flags_ & Message::Type)
        properties_[Message::Type] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::UserId)
        properties_[Message::UserId] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::AppId)
        properties_[Message::AppId] = readAmqpField(in, MetaType::ShortString);

    if (flags_ & Message::ClusterID)
        properties_[Message::ClusterID] = readAmqpField(in, MetaType::ShortString);
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
