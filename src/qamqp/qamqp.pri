QT += network

DEPENDPATH += $$PWD

HEADERS += $$PWD/amqp.h \
           $$PWD/amqp_authenticator.h \
           $$PWD/amqp_channel.h \
           $$PWD/amqp_channel_p.h \
           $$PWD/amqp_connection.h \
           $$PWD/amqp_connection_p.h \
           $$PWD/amqp_exchange.h \
           $$PWD/amqp_exchange_p.h \
           $$PWD/amqp_frame.h \
           $$PWD/amqp_message.h \
           $$PWD/amqp_network.h \
           $$PWD/amqp_p.h \
           $$PWD/amqp_queue.h \
           $$PWD/amqp_queue_p.h \
           $$PWD/amqp_global.h \

SOURCES += $$PWD/amqp.cpp \
           $$PWD/amqp_authenticator.cpp \
           $$PWD/amqp_channel.cpp \
           $$PWD/amqp_connection.cpp \
           $$PWD/amqp_exchange.cpp \
           $$PWD/amqp_frame.cpp \
           $$PWD/amqp_network.cpp \
           $$PWD/amqp_queue.cpp \