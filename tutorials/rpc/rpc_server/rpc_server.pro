DEPTH = ../../..
include($${DEPTH}/qamqp.pri)

TEMPLATE = app
INCLUDEPATH += $${QAMQP_INCLUDEPATH}
LIBS += -L$${DEPTH}/src $${QAMQP_LIBS}
macx:CONFIG -= app_bundle

HEADERS += \
    server.h
SOURCES += \
    server.cpp \
    main.cpp
