#ifndef qamqp_global_h__
#define qamqp_global_h__

#define QAMQP_P_INCLUDE
#define AMQPSCHEME "amqp"
#define AMQPSSCHEME "amqps"
#define AMQPPORT 5672
#define AMQPHOST "localhost"
#define AMQPVHOST "/"
#define AMQPLOGIN "guest"
#define AMQPPSWD  "guest"
#define FRAME_MAX 131072

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

#endif // qamqp_global_h__
