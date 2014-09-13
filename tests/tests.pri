INCLUDEPATH += $${QAMQP_INCLUDEPATH} $${PWD}/common
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}

unix:!macx:QMAKE_RPATHDIR += $${OUT_PWD}/$${DEPTH}/src
macx {
    QMAKE_RPATHDIR += @loader_path/$${DEPTH}/src
    QMAKE_LFLAGS += -Wl,-rpath,@loader_path/$${DEPTH}/src
}

QT = core network testlib
QT -= gui
CONFIG -= app_bundle
CONFIG += testcase

HEADERS += \
    $${PWD}/common/signalspy.h \
    $${PWD}/common/qamqptestcase.h

