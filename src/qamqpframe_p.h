#ifndef QAMQPFRAME_P_H
#define QAMQPFRAME_P_H

#include <QDataStream>
#include <QReadWriteLock>
#include <QHash>
#include <QVariant>

#include "qamqpglobal.h"
#include "qamqpmessage.h"

class QAmqpFramePrivate;
class QAmqpFrame
{
public:
    static const qint64 HEADER_SIZE = 7;
    static const qint64 FRAME_END_SIZE = 1;
    static const quint8 FRAME_END = 0xCE;

    enum FrameType
    {
        Method = 1,
        Header = 2,
        Body = 3,
        Heartbeat = 8
    };

    enum MethodClass
    {
        Connection = 10,
        Channel = 20,
        Exchange = 40,
        Queue = 50,
        Basic = 60,
        Confirm = 85,
        Tx = 90
    };

    virtual ~QAmqpFrame();

    FrameType type() const;

    quint16 channel() const;
    void setChannel(quint16 channel);

    static int writeTimeout();
    static void setWriteTimeout(int msecs);

    virtual qint32 size() const;

    static QVariant readAmqpField(QDataStream &s, QAmqpMetaType::ValueType type);
    static void writeAmqpField(QDataStream &s, QAmqpMetaType::ValueType type, const QVariant &value);

protected:
    explicit QAmqpFrame(FrameType type);
    virtual void writePayload(QDataStream &stream) const = 0;
    virtual void readPayload(QDataStream &stream) = 0;

    qint32 size_;

private:
    qint8 type_;
    quint16 channel_;

    static QReadWriteLock lock_;
    static int writeTimeout_;

    friend QDataStream &operator<<(QDataStream &stream, const QAmqpFrame &frame);
    friend QDataStream &operator>>(QDataStream &stream, QAmqpFrame &frame);
};

QDataStream &operator<<(QDataStream &, const QAmqpFrame &frame);
QDataStream &operator>>(QDataStream &, QAmqpFrame &frame);

class QAMQP_EXPORT QAmqpMethodFrame : public QAmqpFrame
{
public:
    QAmqpMethodFrame();
    QAmqpMethodFrame(MethodClass methodClass, qint16 id);

    qint16 id() const;
    MethodClass methodClass() const;

    virtual qint32 size() const;

    QByteArray arguments() const;
    void setArguments(const QByteArray &data);

private:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);

    short methodClass_;
    qint16 id_;
    QByteArray arguments_;
};

class QAmqpContentFrame : public QAmqpFrame
{
public:
    QAmqpContentFrame();
    QAmqpContentFrame(MethodClass methodClass);

    MethodClass methodClass() const;

    virtual qint32 size() const;

    QVariant property(QAmqpMessage::Property prop) const;
    void setProperty(QAmqpMessage::Property prop, const QVariant &value);

    qlonglong bodySize() const;
    void setBodySize(qlonglong size);

private:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);
    friend class QAmqpQueuePrivate;

    short methodClass_;
    qint16 id_;
    mutable QByteArray buffer_;
    QAmqpMessage::PropertyHash properties_;
    qlonglong bodySize_;
};

class QAmqpContentBodyFrame : public QAmqpFrame
{
public:
    QAmqpContentBodyFrame();

    void setBody(const QByteArray &data);
    QByteArray body() const;

    virtual qint32 size() const;

private:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);

    QByteArray body_;
};

class QAmqpHeartbeatFrame : public QAmqpFrame
{
public:
    QAmqpHeartbeatFrame();

private:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);
};

class QAmqpMethodFrameHandler
{
public:
    virtual bool _q_method(const QAmqpMethodFrame &frame) = 0;
};

class QAmqpContentFrameHandler
{
public:
    virtual void _q_content(const QAmqpContentFrame &frame) = 0;
};

class QAmqpContentBodyFrameHandler
{
public:
    virtual void _q_body(const QAmqpContentBodyFrame &frame) = 0;
};

#endif // QAMQPFRAME_P_H
