QAMQP_VERSION = 0.6.0

isEmpty(QAMQP_LIBRARY_TYPE) {
    QAMQP_LIBRARY_TYPE = shared
}

QT += network
QAMQP_INCLUDEPATH = $${PWD}/src
QAMQP_LIBS = -lqamqp
CONFIG(debug, debug|release){
    win32:QAMQP_LIBS = -lqamqpd
}
contains(QAMQP_LIBRARY_TYPE, staticlib) {
    DEFINES += QAMQP_STATIC
} else {
    DEFINES += QAMQP_SHARED
    win32:QAMQP_LIBS = -lqamqp0
}

isEmpty(PREFIX) {
    unix {
        PREFIX = /usr
    } else {
        PREFIX = $$[QT_INSTALL_PREFIX]
    }
}
isEmpty(LIBDIR) {
    LIBDIR = lib
}

