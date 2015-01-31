DEPTH = ../../..
include($${DEPTH}/qamqp.pri)
include($${DEPTH}/tests/tests.pri)

TARGET = tst_qamqpclient
SOURCES = tst_qamqpclient.cpp
RESOURCES = certs.qrc
