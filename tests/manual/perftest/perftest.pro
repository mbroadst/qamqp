DEPTH = ../../..
include($${DEPTH}/qamqp.pri)
include($${DEPTH}/tests/tests.pri)
include(qcommandlineparser/qcommandlineparser.pri)

TEMPLATE = app
TARGET = perftest
CONFIG -= app_bundle
QT += testlib

HEADERS += \
    producer.h \
    consumer.h \
    testoptions.h
SOURCES += \
    producer.cpp \
    consumer.cpp \
    testoptions.cpp \
    main.cpp
