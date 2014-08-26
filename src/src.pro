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
    qamqpchannel_p.h \
    qamqpclient_p.h \
    qamqpexchange_p.h \
    qamqpframe_p.h \
    qamqpmessage_p.h \
    qamqpqueue_p.h

INSTALL_HEADERS += \
    qamqpauthenticator.h \
    qamqpchannel.h \
    qamqpclient.h \
    qamqpexchange.h \
    qamqpglobal.h \
    qamqpmessage.h \
    qamqpqueue.h \
    qamqptable.h

HEADERS += \
    $${INSTALL_HEADERS} \
    $${PRIVATE_HEADERS}

SOURCES += \
    qamqpauthenticator.cpp \
    qamqpchannel.cpp \
    qamqpclient.cpp \
    qamqpexchange.cpp \
    qamqpframe.cpp \
    qamqpmessage.cpp \
    qamqpqueue.cpp \
    qamqptable.cpp

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
