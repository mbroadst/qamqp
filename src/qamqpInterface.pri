INCLUDEPATH += $${PWD}

CONFIG(debug, debug|release){
    win32:LIBS += -L$$PWD -lqamqpd
    unix:LIBS += -L$$PWD -lqamqp
} else {
    LIBS += -L$$PWD -lqamqp
}
