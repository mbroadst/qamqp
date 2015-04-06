DEPTH = ../../..
include($${DEPTH}/qamqp.pri)
include($${DEPTH}/tests/tests.pri)

TARGET = tst_qamqpqueue
SOURCES = tst_qamqpqueue.cpp Issue23.cpp
HEADERS = Issue23.h
