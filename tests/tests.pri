INCLUDEPATH += $${QAMQP_INCLUDEPATH}
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}
QT = core network testlib
QT -= gui
CONFIG -= app_bundle
CONFIG += testcase
