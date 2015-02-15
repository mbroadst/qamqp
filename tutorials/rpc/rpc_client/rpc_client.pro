DEPTH = ../../..
include($${DEPTH}/qamqp.pri)

TEMPLATE = app
INCLUDEPATH += $${QAMQP_INCLUDEPATH}
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}
macx:CONFIG -= app_bundle

HEADERS += \
    fibonaccirpcclient.h
SOURCES += \
    fibonaccirpcclient.cpp \
    main.cpp
