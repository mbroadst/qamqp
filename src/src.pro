include(../qamqp.pri)

INCLUDEPATH += .
TEMPLATE = lib
TARGET = qamqp
QT += core network
QT -= gui
DEFINES += QAMQP_BUILD
CONFIG += $${QAMQP_LIBRARY_TYPE}
VERSION = $${QAMQP_VERSION}
win32:DESTDIR = $$OUT_PWD

PRIVATE_HEADERS += \
    amqp_channel_p.h \
    amqp_client_p.h \
    amqp_connection_p.h \
    amqp_exchange_p.h \
    amqp_network_p.h \
    amqp_queue_p.h

INSTALL_HEADERS += \
    amqp_authenticator.h \
    amqp_channel.h \
    amqp_client.h \
    amqp_exchange.h \
    amqp_frame.h \
    amqp_global.h \
    amqp_message.h \
    amqp_queue.h

HEADERS += \
    $${INSTALL_HEADERS} \
    $${PRIVATE_HEADERS}

SOURCES += \
    amqp_authenticator.cpp \
    amqp_channel.cpp \
    amqp_client.cpp \
    amqp_connection.cpp \
    amqp_exchange.cpp \
    amqp_frame.cpp \
    amqp_message.cpp \
    amqp_network.cpp \
    amqp_queue.cpp

# install
headers.files = $${INSTALL_HEADERS}
headers.path = $${PREFIX}/include/qamqp
target.path = $${PREFIX}/$${LIBDIR}
INSTALLS += headers target

# pkg-config support
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
equals(QAMQP_LIBRARY_TYPE, staticlib) {
    QMAKE_PKGCONFIG_CFLAGS = -DQAMQP_STATIC
} else {
    QMAKE_PKGCONFIG_CFLAGS = -DQAMQP_SHARED
}
unix:QMAKE_CLEAN += -r pkgconfig lib$${TARGET}.prl
