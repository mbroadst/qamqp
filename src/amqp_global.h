#ifndef qamqp_global_h__
#define qamqp_global_h__

#define AMQP_SCHEME "amqp"
#define AMQP_SSCHEME "amqps"
#define AMQP_PORT 5672
#define AMQP_HOST "localhost"
#define AMQP_VHOST "/"
#define AMQP_LOGIN "guest"
#define AMQP_PSWD  "guest"

#define AMQP_FRAME_MAX 131072
#define AMQP_FRAME_MIN_SIZE 4096

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

#endif // qamqp_global_h__
