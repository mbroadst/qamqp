INCLUDEPATH += $${QAMQP_INCLUDEPATH} $${PWD}/common
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}
QT = core network testlib
QT -= gui
CONFIG -= app_bundle
CONFIG += testcase

HEADERS += $${PWD}/common/signalspy.h
