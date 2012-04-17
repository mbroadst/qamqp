TEMPLATE = app
QT += network
TARGET = qamqp
DEPENDPATH += . \
              src \
              src/qamqp
INCLUDEPATH += . ./src

# Input
HEADERS += src/test.h \
           src/qamqp/amqp.h \
           src/qamqp/amqp_authenticator.h \
           src/qamqp/amqp_channel.h \
           src/qamqp/amqp_channel_p.h \
           src/qamqp/amqp_connection.h \
           src/qamqp/amqp_connection_p.h \
           src/qamqp/amqp_exchange.h \
           src/qamqp/amqp_exchange_p.h \
           src/qamqp/amqp_frame.h \
           src/qamqp/amqp_message.h \
           src/qamqp/amqp_network.h \
           src/qamqp/amqp_p.h \
           src/qamqp/amqp_queue.h \
           src/qamqp/amqp_queue_p.h \
           src/qamqp/qamqp_global.h \

SOURCES += src/main.cpp \
           src/test.cpp \
           src/qamqp/amqp.cpp \
           src/qamqp/amqp_authenticator.cpp \
           src/qamqp/amqp_channel.cpp \
           src/qamqp/amqp_connection.cpp \
           src/qamqp/amqp_exchange.cpp \
           src/qamqp/amqp_frame.cpp \
           src/qamqp/amqp_network.cpp \
           src/qamqp/amqp_queue.cpp \

