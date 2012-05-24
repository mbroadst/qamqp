TEMPLATE = app
TARGET = qamqp
DEPENDPATH += . \
              src 
			  
INCLUDEPATH += . ./src

# Input
HEADERS += src/test.h

SOURCES += src/main.cpp \
           src/test.cpp
		   
include(src/qamqp/qamqp.pri)