#ifndef QAMQPFRAME_P_H
#define QAMQPFRAME_P_H

#include <QDataStream>
#include <QHash>
#include <QVariant>

#include "qamqpglobal.h"
#include "qamqpmessage.h"

class QAmqpQueuePrivate;

/**
 *  Library namespace
 *  @namespace QAMQP
 */

class QAmqpFrame
{
public:
    /*
     * @brief Header size in bytes
     */
    static const qint64 HEADER_SIZE = 7;

    /*
     * @brief Frame end indicator size in bytes
    */
    static const qint64 FRAME_END_SIZE = 1;

    /*
     * @brief Frame end marker
     */
    static const quint8 FRAME_END = 0xCE;

    /*
     * @brief Frame type
     */
    enum Type
    {
        ftMethod = 1, /*!< Used define method frame */
        ftHeader = 2, /*!< Used define content header frame */
        ftBody = 3,   /*!< Used define content body frame */
        ftHeartbeat = 8 /*!< Used define heartbeat frame */
    };

    /*
     * @brief Frame method class
     * @enum MethodClass
     */
    enum MethodClass
    {
        fcConnection = 10,  // Define class of methods related to connection
        fcChannel = 20,     // Define class of methods related to channel
        fcExchange = 40,    // Define class of methods related to exchange
        fcQueue = 50,       // Define class of methods related to queue
        fcBasic = 60,       // Define class of methods related to basic command
        fcTx = 90,
    };

    static QVariant readAmqpField(QDataStream &s, QAmqpMetaType::ValueType type);
    static void writeAmqpField(QDataStream &s, QAmqpMetaType::ValueType type, const QVariant &value);

    /*
     * @brief Base class for any frames.
     * @detailed Implement main methods for serialize and deserialize raw frame data.
     * All frames start with a 7-octet header composed of a type field (octet), a channel field (short integer) and a
     * size field (long integer):
     * @code Frame struct
     *     0	  1		 3	   7				  size+7	size+8
     *     +------+---------+---------+ +-------------+ +-----------+
     *     | type | channel | size	| | payload	 | | frame-end |
     *     +------+---------+---------+ +-------------+ +-----------+
     *   @endcode
     *   octet short long 'size' octets octet
     */

    /*
     * Base class constructor.
     * @detailed Construct frame class for sending.
     * @param type Define type of constructed frame.
     */
    QAmqpFrame(Type type);

    /*
     * Base class constructor.
     * @detailed Construct frame class from received raw data.
     * @param raw Data stream for reading source data.
     */
    QAmqpFrame(QDataStream &raw);

    /*
     * Base class virtual destructor
     */
    virtual ~QAmqpFrame();

    /*
     * Frame type
     * @detailed Return type of current frame.
     */
    Type type() const;

    /*
     * Set number of associated channel.
     * @param channel Number of channel.
     * @sa channel()
     */
    void setChannel(qint16 channel);

    /*
     * Return number of associated channel.
     * @sa setChannel()
     */
    qint16 channel() const;

    /*
     * Return size of frame.
     */
    virtual qint32 size() const;

    /*
     * Output frame to stream.
     * @param stream Stream for serilize frame.
     */
    void toStream(QDataStream &stream) const;

protected:
    void writeHeader(QDataStream &stream) const;
    virtual void writePayload(QDataStream &stream) const = 0;
    void writeEnd(QDataStream &stream) const;

    void readHeader(QDataStream &stream);
    virtual void readPayload(QDataStream &stream) = 0;

    qint32 size_;

private:
    qint8 type_;
    qint16 channel_;

};

/*
 * @brief Class for working with method frames.
 * @detailed Implement main methods for serialize and deserialize raw method frame data.
 * Method frame bodies consist of an invariant list of data fields, called "arguments". All method bodies start
 * with identifier numbers for the class and method:
 * @code Frame struct
 * 0		   2		   4
 * +----------+-----------+-------------- - -
 * | class-id | method-id | arguments...
 * +----------+-----------+-------------- - -
 *     short	  short	...
 * @endcode
 */
class QAMQP_EXPORT QAmqpMethodFrame : public QAmqpFrame
{
public:
    /*
     * Method class constructor.
     * @detailed Construct frame class for sending.
     * @param methodClass Define method class id of constructed frame.
     * @param id Define method id of constructed frame.
     */
    explicit QAmqpMethodFrame(MethodClass methodClass, qint16 id);

    /*
     * Method class constructor.
     * @detailed Construct frame class from received raw data.
     * @param raw Data stream for reading source data.
     */
    explicit QAmqpMethodFrame(QDataStream &raw);

    /*
     * Method class type.
     */
    MethodClass methodClass() const;

    /*
     * Method id.
     */
    qint16 id() const;
    qint32 size() const;

    /*
     * Set arguments for method.
     * @param data Serialized method arguments.
     * @sa arguments
     */
    void setArguments(const QByteArray &data);

    /*
     * Return arguments for method.
     * @sa setArguments
     */
    QByteArray arguments() const;

protected:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);
    short methodClass_;
    qint16 id_;
    QByteArray arguments_;

};

/*
 * @brief Class for working with content frames.
 * @detailed Implement main methods for serialize and deserialize raw content frame data.
 * A content header payload has this format:
 * @code Frame struct
 * +----------+--------+-----------+----------------+------------- - -
 * | class-id | weight | body size | property flags | property list...
 * +----------+--------+-----------+----------------+------------- - -
 *     short     short   long long       short          remainder...
 * @endcode
 *
 * | Property         | Description                            |
 * | ---------------- | -------------------------------------- |
 * | ContentType      | MIME content type                      |
 * | ContentEncoding  | MIME content encoding                  |
 * | Headers          | message header field table             |
 * | DeliveryMode     | nonpersistent (1) or persistent (2)    |
 * | Priority         | message priority, 0 to 9               |
 * | CorrelationId    | application correlation identifier     |
 * | ReplyTo          | address to reply to                    |
 * | Expiration       | message expiration specification       |
 * | MessageId        | application message identifier         |
 * | Timestamp        | message timestamp                      |
 * | Type             | message type name                      |
 * | UserId           | creating user id                       |
 * | AppId            | creating application id                |
 * | ClusterID        | cluster ID                             |
 *
 * Default property:
 * @sa setProperty
 * @sa property
 */
class QAMQP_EXPORT QAmqpContentFrame : public QAmqpFrame
{
public:
    /*
     * Content class constructor.
     * @detailed Construct frame content header class for sending.
     */
    QAmqpContentFrame();

    /*
     * Content class constructor.
     * @detailed Construct frame content header class for sending.
     * @param methodClass Define method class id of constructed frame.
     */
    QAmqpContentFrame(MethodClass methodClass);

    /*
     * Content class constructor.
     * @detailed Construct frame content header class for sending.
     * @param raw Data stream for reading source data.
     */
    QAmqpContentFrame(QDataStream &raw);

    /*
     * Method class type.
     */
    MethodClass methodClass() const;
    qint32 size() const;

    /*
     * Set default content header property
     * @param prop Any default content header property
     * @param value Associated data
     */
    void setProperty(QAmqpMessage::Property prop, const QVariant &value);

    /*
     * Return associated with property value
     * @param prop Any default content header property
     */
    QVariant property(QAmqpMessage::Property prop) const;

    qlonglong bodySize() const;
    void setBodySize(qlonglong size);

protected:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);
    short methodClass_;
    qint16 id_;
    mutable QByteArray buffer_;
    QAmqpMessage::PropertyHash properties_;
    qlonglong bodySize_;

private:
    friend class QAmqpQueuePrivate;

};

class QAMQP_EXPORT QAmqpContentBodyFrame : public QAmqpFrame
{
public:
    QAmqpContentBodyFrame();
    QAmqpContentBodyFrame(QDataStream &raw);

    void setBody(const QByteArray &data);
    QByteArray body() const;

    qint32 size() const;
protected:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);

private:
    QByteArray body_;
};

/*
 * @brief Class for working with heartbeat frames.
 * @detailed Implement frame for heartbeat send.
 */
class QAMQP_EXPORT QAmqpHeartbeatFrame : public QAmqpFrame
{
public:
    /*
     * Heartbeat class constructor.
     * @detailed Construct frame class for sending.
     */
    QAmqpHeartbeatFrame();

protected:
    void writePayload(QDataStream &stream) const;
    void readPayload(QDataStream &stream);
};

class QAMQP_EXPORT QAmqpMethodFrameHandler
{
public:
    virtual bool _q_method(const QAmqpMethodFrame &frame) = 0;
};

class QAMQP_EXPORT QAmqpContentFrameHandler
{
public:
    virtual void _q_content(const QAmqpContentFrame &frame) = 0;
};

class QAMQP_EXPORT QAmqpContentBodyFrameHandler
{
public:
    virtual void _q_body(const QAmqpContentBodyFrame &frame) = 0;
};

#endif // QAMQPFRAME_P_H
