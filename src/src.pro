include(../qamqp.pri)

INCLUDEPATH += .
TEMPLATE = lib
TARGET = qamqp
build_pass:CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}
QT += core network
QT -= gui
DEFINES += QAMQP_BUILD
CONFIG += $${QAMQP_LIBRARY_TYPE}
VERSION = $${QAMQP_VERSION}
win32:DESTDIR = $$OUT_PWD
macx:QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/

# for some reason with Travis' qt 5.0.2 you can't chain these with an |
NEED_GCOV_SUPPORT = 0
greaterThan(QT_MAJOR_VERSION, 4):lessThan(QT_MINOR_VERSION, 2) {
    NEED_GCOV_SUPPORT = 1
}
lessThan(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9):lessThan(QT_PATCH_VERSION, 6) {
    NEED_GCOV_SUPPORT = 1
}

greaterThan(NEED_GCOV_SUPPORT, 0) {
    # NOTE: remove when travis adds a newer ubuntu, or when hell freezes over
    gcov {
        QMAKE_CFLAGS           += -fprofile-arcs -ftest-coverage
        QMAKE_CXXFLAGS         += -fprofile-arcs -ftest-coverage
        QMAKE_OBJECTIVE_CFLAGS += -fprofile-arcs -ftest-coverage
        QMAKE_LFLAGS           += -fprofile-arcs -ftest-coverage
        QMAKE_CLEAN += $(OBJECTS_DIR)*.gcno and $(OBJECTS_DIR)*.gcda
    }
}

#Define GIT Macros
GIT_VERSION = $$system(git describe --long --dirty --tags)
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"

GIT_TAG = $$system(git describe --abbrev=0)
VERSION = $$replace(GIT_TAG, v,)

PRIVATE_HEADERS += \
    qamqpchannel_p.h \
    qamqpchannelhash_p.h \
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
    qamqpchannelhash.cpp \
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
