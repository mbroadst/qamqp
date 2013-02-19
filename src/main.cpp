#include <QtCore/QCoreApplication>

#include "QamqpApp.h"


int main(int argc, char *argv[])
{
    QAMQP::samples::QamqpApp qamqpApp(argc, argv);

    return qamqpApp.exec();
}
