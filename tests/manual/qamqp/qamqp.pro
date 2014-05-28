DEPTH = ../../..
include($${DEPTH}/qamqp.pri)
include($${DEPTH}/tests/tests.pri)
CONFIG -= testcase

TEMPLATE = app
TARGET = qamqp
HEADERS += \
    QamqpApp.h \
    pubsub/EmitLog.h \
    pubsub/ReceiveLog.h \
    routing/EmitLogDirect.h \
    routing/ReceiveLogDirect.h \
    sendreceive/Receive.h \
    sendreceive/Send.h \
    workqueues/NewTask.h \
    workqueues/Worker.h

SOURCES += \
    main.cpp
