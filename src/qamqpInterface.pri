INCLUDEPATH += $${PWD}

CONFIG(debug, debug|release){
    LIBS += -L$$PWD -lqamqpd
} else {
    LIBS += -L$$PWD -lqamqp
}
