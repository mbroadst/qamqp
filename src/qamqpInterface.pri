INCLUDEPATH += $${PWD}

CONFIG(debug, debug|release){
    LIBS += -L$$PWD -lqamqp
} else {
    LIBS += -L$$PWD -lqamqp
}
