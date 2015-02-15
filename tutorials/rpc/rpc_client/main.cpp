#include <QCoreApplication>
#include "fibonaccirpcclient.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    FibonacciRpcClient client;
    if (!client.connectToServer())
        return EXIT_FAILURE;

    client.call(30);
    return app.exec();
}
