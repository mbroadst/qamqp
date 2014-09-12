INCLUDEPATH += $${QAMQP_INCLUDEPATH} $${PWD}/common
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}

QMAKE_RPATHDIR += $${DEPTH}/src

QT = core network testlib
QT -= gui
CONFIG -= app_bundle
CONFIG += testcase

HEADERS += \
    $${PWD}/common/signalspy.h \
    $${PWD}/common/qamqptestcase.h

