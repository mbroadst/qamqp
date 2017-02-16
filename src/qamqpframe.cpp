#include <QDateTime>
#include <QList>
#include <QDebug>

#include "qamqptable.h"
#include "qamqpglobal.h"
#include "qamqpframe_p.h"

QReadWriteLock QAmqpFrame::lock_;
int QAmqpFrame::writeTimeout_ = 1000;

QAmqpFrame::QAmqpFrame(FrameType type)
    : size_(0),
      type_(type),
      channel_(0)
{
}

QAmqpFrame::FrameType QAmqpFrame::type() const
{
    return static_cast<QAmqpFrame::FrameType>(type_);
}

QAmqpFrame::~QAmqpFrame()
{
}

void QAmqpFrame::setChannel(quint16 channel)
{
    channel_ = channel;
}

int QAmqpFrame::writeTimeout()
{
    QReadLocker locker(&lock_);
    return writeTimeout_;
}

void QAmqpFrame::setWriteTimeout(int msecs)
{
    QWriteLocker locker(&lock_);
    writeTimeout_ = msecs;
}

quint16 QAmqpFrame::channel() const
{
    return channel_;
}

qint32 QAmqpFrame::size() const
{
    return 0;
}

/*
void QAmqpFrame::readEnd(QDataStream &stream)
{
    unsigned char end_  = 0;
    stream.readRawData(reinterpret_cast<char*>(&end_), sizeof(end_));
    if (end_ != FRAME_END)
        qAmqpDebug("Wrong end of frame");
}
*/

QDataStream &operator<<(QDataStream &stream, const QAmqpFrame &frame)
{
    // write header
    stream << frame.type_;
    stream << frame.channel_;
    stream << frame.size();

    frame.writePayload(stream);

    // write end
    stream << qint8(QAmqpFrame::FRAME_END);
    
    int writeTimeout = QAmqpFrame::writeTimeout();
    if(writeTimeout >= -1)
    {
        stream.device()->waitForBytesWritten(writeTimeout);
    }

    return stream;
}

QDataStream &operator>>(QDataStream &stream, QAmqpFrame &frame)
{
    stream >> frame.type_;
    stream >> frame.channel_;
    stream >> frame.size_;
    frame.readPayload(stream);
    return stream;
}

//////////////////////////////////////////////////////////////////////////

QAmqpMethodFrame::QAmqpMethodFrame()
    : QAmqpFrame(QAmqpFrame::Method),
      methodClass_(0),
      id_(0)
{
}

QAmqpMethodFrame::QAmqpMethodFrame(MethodClass methodClass, qint16 id)
    : QAmqpFrame(QAmqpFrame::Method),
      methodClass_(methodClass),
      id_(id)
{
}

QAmqpFrame::MethodClass QAmqpMethodFrame::methodClass() const
{
    return MethodClass(methodClass_);
}

qint16 QAmqpMethodFrame::id() const
{
    return id_;
}

qint32 QAmqpMethodFrame::size() const
{
    return sizeof(id_) + sizeof(methodClass_) + arguments_.size();
}

void QAmqpMethodFrame::setArguments(const QByteArray &data)
{
    arguments_ = data;
}

QByteArray QAmqpMethodFrame::arguments() const
{
    return arguments_;
}

void QAmqpMethodFrame::readPayload(QDataStream &stream)
{
    stream >> methodClass_;
    stream >> id_;

    arguments_.resize(size_ - (sizeof(id_) + sizeof(methodClass_)));
    stream.readRawData(arguments_.data(), arguments_.size());
}

void QAmqpMethodFrame::writePayload(QDataStream &stream) const
{
    stream << quint16(methodClass_);
    stream << quint16(id_);
    stream.writeRawData(arguments_.data(), arguments_.size());
}

//////////////////////////////////////////////////////////////////////////

QVariant QAmqpFrame::readAmqpField(QDataStream &s, QAmqpMetaType::ValueType type)
{
    switch (type) {
    case QAmqpMetaType::Boolean:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<bool>(octet > 0);
    }
    case QAmqpMetaType::ShortShortUint:
    {
        quint8 octet = 0;
        s >> octet;
        return QVariant::fromValue<int>(octet);
    }
    case QAmqpMetaType::ShortUint:
    {
        quint16 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case QAmqpMetaType::LongUint:
    {
        quint32 tmp_value = 0;
        s >> tmp_value;
        return QVariant::fromValue<uint>(tmp_value);
    }
    case QAmqpMetaType::LongLongUint:
    {
        qulonglong v = 0 ;
        s >> v;
        return v;
    }
    case QAmqpMetaType::ShortString:
    {
        qint8 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromLatin1(buffer.data(), size);
    }
    case QAmqpMetaType::LongString:
    {
        quint32 size = 0;
        QByteArray buffer;

        s >> size;
        buffer.resize(size);
        s.readRawData(buffer.data(), buffer.size());
        return QString::fromUtf8(buffer.data(), buffer.size());
    }
    case QAmqpMetaType::Timestamp:
    {
        qulonglong tmp_value;
        s >> tmp_value;
        return QDateTime::fromTime_t(tmp_value);
    }
    case QAmqpMetaType::Hash:
    {
        QAmqpTable table;
        s >> table;
        return table;
    }
    case QAmqpMetaType::Void:
        return QVariant();
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unsupported value type: " << type;
    }

    return QVariant();
}

void QAmqpFrame::writeAmqpField(QDataStream &s, QAmqpMetaType::ValueType type, const QVariant &value)
{
    switch (type) {
    case QAmqpMetaType::Boolean:
        s << (value.toBool() ? qint8(1) : qint8(0));
        break;
    case QAmqpMetaType::ShortShortUint:
        s << qint8(value.toUInt());
        break;
    case QAmqpMetaType::ShortUint:
        s << quint16(value.toUInt());
        break;
    case QAmqpMetaType::LongUint:
        s << quint32(value.toUInt());
        break;
    case QAmqpMetaType::LongLongUint:
        s << qulonglong(value.toULongLong());
        break;
    case QAmqpMetaType::ShortString:
    {
        QString str = value.toString();
        if (str.length() >= 256) {
            qAmqpDebug() << Q_FUNC_INFO << "invalid shortstr length: " << str.length();
        }

        s << quint8(str.length());
        s.writeRawData(str.toUtf8().data(), str.length());
    }
        break;
    case QAmqpMetaType::LongString:
    {
        QString str = value.toString();
        s << quint32(str.length());
        s.writeRawData(str.toLatin1().data(), str.length());
    }
        break;
    case QAmqpMetaType::Timestamp:
        s << qulonglong(value.toDateTime().toTime_t());
        break;
    case QAmqpMetaType::Hash:
    {
        QAmqpTable table(value.toHash());
        s << table;
    }
        break;
    default:
        qAmqpDebug() << Q_FUNC_INFO << "unsupported value type: " << type;
    }
}

//////////////////////////////////////////////////////////////////////////

QAmqpContentFrame::QAmqpContentFrame()
    : QAmqpFrame(QAmqpFrame::Header)
{
}

QAmqpContentFrame::QAmqpContentFrame(QAmqpFrame::MethodClass methodClass)
    : QAmqpFrame(QAmqpFrame::Header)
{
    methodClass_ = methodClass;
}

QAmqpFrame::MethodClass QAmqpContentFrame::methodClass() const
{
    return MethodClass(methodClass_);
}

qint32 QAmqpContentFrame::size() const
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

    if (prop_ & QAmqpMessage::ContentType)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::ContentType]);

    if (prop_ & QAmqpMessage::ContentEncoding)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::ContentEncoding]);

    if (prop_ & QAmqpMessage::Headers)
        writeAmqpField(out, QAmqpMetaType::Hash, properties_[QAmqpMessage::Headers]);

    if (prop_ & QAmqpMessage::DeliveryMode)
        writeAmqpField(out, QAmqpMetaType::ShortShortUint, properties_[QAmqpMessage::DeliveryMode]);

    if (prop_ & QAmqpMessage::Priority)
        writeAmqpField(out, QAmqpMetaType::ShortShortUint, properties_[QAmqpMessage::Priority]);

    if (prop_ & QAmqpMessage::CorrelationId)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::CorrelationId]);

    if (prop_ & QAmqpMessage::ReplyTo)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::ReplyTo]);

    if (prop_ & QAmqpMessage::Expiration)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::Expiration]);

    if (prop_ & QAmqpMessage::MessageId)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::MessageId]);

    if (prop_ & QAmqpMessage::Timestamp)
        writeAmqpField(out, QAmqpMetaType::Timestamp, properties_[QAmqpMessage::Timestamp]);

    if (prop_ & QAmqpMessage::Type)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::Type]);

    if (prop_ & QAmqpMessage::UserId)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::UserId]);

    if (prop_ & QAmqpMessage::AppId)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::AppId]);

    if (prop_ & QAmqpMessage::ClusterID)
        writeAmqpField(out, QAmqpMetaType::ShortString, properties_[QAmqpMessage::ClusterID]);

    return buffer_.size();
}

qlonglong QAmqpContentFrame::bodySize() const
{
    return bodySize_;
}

void QAmqpContentFrame::setBodySize(qlonglong size)
{
    bodySize_ = size;
}

void QAmqpContentFrame::setProperty(QAmqpMessage::Property prop, const QVariant &value)
{
    properties_[prop] = value;
}

QVariant QAmqpContentFrame::property(QAmqpMessage::Property prop) const
{
    return properties_.value(prop);
}

void QAmqpContentFrame::writePayload(QDataStream &out) const
{
    out.writeRawData(buffer_.data(), buffer_.size());
}

void QAmqpContentFrame::readPayload(QDataStream &in)
{
    in >> methodClass_;
    in.skipRawData(2); //weight
    in >> bodySize_;
    qint16 flags_ = 0;
    in >> flags_;
    if (flags_ & QAmqpMessage::ContentType)
        properties_[QAmqpMessage::ContentType] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::ContentEncoding)
        properties_[QAmqpMessage::ContentEncoding] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::Headers)
        properties_[QAmqpMessage::Headers] = readAmqpField(in, QAmqpMetaType::Hash);

    if (flags_ & QAmqpMessage::DeliveryMode)
        properties_[QAmqpMessage::DeliveryMode] = readAmqpField(in, QAmqpMetaType::ShortShortUint);

    if (flags_ & QAmqpMessage::Priority)
        properties_[QAmqpMessage::Priority] = readAmqpField(in, QAmqpMetaType::ShortShortUint);

    if (flags_ & QAmqpMessage::CorrelationId)
        properties_[QAmqpMessage::CorrelationId] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::ReplyTo)
        properties_[QAmqpMessage::ReplyTo] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::Expiration)
        properties_[QAmqpMessage::Expiration] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::MessageId)
        properties_[QAmqpMessage::MessageId] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::Timestamp)
        properties_[QAmqpMessage::Timestamp] = readAmqpField(in, QAmqpMetaType::Timestamp);

    if (flags_ & QAmqpMessage::Type)
        properties_[QAmqpMessage::Type] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::UserId)
        properties_[QAmqpMessage::UserId] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::AppId)
        properties_[QAmqpMessage::AppId] = readAmqpField(in, QAmqpMetaType::ShortString);

    if (flags_ & QAmqpMessage::ClusterID)
        properties_[QAmqpMessage::ClusterID] = readAmqpField(in, QAmqpMetaType::ShortString);
}

//////////////////////////////////////////////////////////////////////////

QAmqpContentBodyFrame::QAmqpContentBodyFrame()
    : QAmqpFrame(QAmqpFrame::Body)
{
}

void QAmqpContentBodyFrame::setBody(const QByteArray &data)
{
    body_ = data;
}

QByteArray QAmqpContentBodyFrame::body() const
{
    return body_;
}

void QAmqpContentBodyFrame::writePayload(QDataStream &out) const
{
    out.writeRawData(body_.data(), body_.size());
}

void QAmqpContentBodyFrame::readPayload(QDataStream &in)
{
    body_.resize(size_);
    in.readRawData(body_.data(), body_.size());
}

qint32 QAmqpContentBodyFrame::size() const
{
    return body_.size();
}

//////////////////////////////////////////////////////////////////////////

QAmqpHeartbeatFrame::QAmqpHeartbeatFrame()
    : QAmqpFrame(QAmqpFrame::Heartbeat)
{
}

void QAmqpHeartbeatFrame::readPayload(QDataStream &stream)
{
    Q_UNUSED(stream)
}

void QAmqpHeartbeatFrame::writePayload(QDataStream &stream) const
{
    Q_UNUSED(stream)
}
