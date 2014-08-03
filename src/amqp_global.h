#ifndef qamqp_global_h__
#define qamqp_global_h__

#include <QMetaType>

#define AMQP_SCHEME "amqp"
#define AMQP_SSCHEME "amqps"
#define AMQP_PORT 5672
#define AMQP_HOST "localhost"
#define AMQP_VHOST "/"
#define AMQP_LOGIN "guest"
#define AMQP_PSWD  "guest"

#define AMQP_FRAME_MAX 131072
#define AMQP_FRAME_MIN_SIZE 4096

#define AMQP_BASIC_CONTENT_TYPE_FLAG (1 << 15)
#define AMQP_BASIC_CONTENT_ENCODING_FLAG (1 << 14)
#define AMQP_BASIC_HEADERS_FLAG (1 << 13)
#define AMQP_BASIC_DELIVERY_MODE_FLAG (1 << 12)
#define AMQP_BASIC_PRIORITY_FLAG (1 << 11)
#define AMQP_BASIC_CORRELATION_ID_FLAG (1 << 10)
#define AMQP_BASIC_REPLY_TO_FLAG (1 << 9)
#define AMQP_BASIC_EXPIRATION_FLAG (1 << 8)
#define AMQP_BASIC_MESSAGE_ID_FLAG (1 << 7)
#define AMQP_BASIC_TIMESTAMP_FLAG (1 << 6)
#define AMQP_BASIC_TYPE_FLAG (1 << 5)
#define AMQP_BASIC_USER_ID_FLAG (1 << 4)
#define AMQP_BASIC_APP_ID_FLAG (1 << 3)
#define AMQP_BASIC_CLUSTER_ID_FLAG (1 << 2)

#define QAMQP_VERSION "0.3.0"

#define AMQP_CONNECTION_FORCED 320

#ifdef QAMQP_SHARED
#   ifdef QAMQP_BUILD
#       define QAMQP_EXPORT Q_DECL_EXPORT
#   else
#       define QAMQP_EXPORT Q_DECL_IMPORT
#   endif
#else
#   define QAMQP_EXPORT
#endif

#define qAmqpDebug if (qgetenv("QAMQP_DEBUG").isEmpty()); else qDebug

namespace QAMQP {

enum ValueType
{
    Invalid = -1,

    // basic AMQP types
    Boolean,
    ShortUint,
    LongUint,
    LongLongUint,
    ShortString,
    LongString,

    // field-value types
    ShortShortInt,
    ShortShortUint,
    ShortInt,
    LongInt,
    LongLongInt,
    Float,
    Double,
    Decimal,
    Array,
    Timestamp,
    Hash,
    Bytes,
    Void
};

enum Error
{
    NoError = 0,
    ContentTooLargeError = 311,
    UnroutableKey = 312,
    NoConsumersError = 313,
    ConnectionForcedError = 320,
    InvalidPathError = 402,
    AccessRefusedError = 403,
    NotFoundError = 404,
    ResourceLockedError = 405,
    PreconditionFailedError = 406,
    FrameError = 501,
    SyntaxError = 502,
    CommandInvalidError = 503,
    ChannelError = 504,
    UnexpectedFrameError = 505,
    ResourceError = 506,
    NotAllowedError = 530,
    NotImplementedError = 540,
    InternalError = 541
};

}   // namespace QAMQP

Q_DECLARE_METATYPE(QAMQP::Error);

#endif // qamqp_global_h__
