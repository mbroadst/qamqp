TEMPLATE = app
TARGET = qamqp
DEPENDPATH += . src

INCLUDEPATH += . ./src

HEADERS += \
  src/QamqpApp.h \
  src/sendreceive/Receive.h \
  src/sendreceive/Send.h \
  src/workqueues/NewTask.h \
  src/workqueues/Worker.h \

SOURCES += \
  src/main.cpp \

include(src/qamqp/qamqp.pri)
