#include <QCoreApplication>
#include <QDebug>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qDebug() << "testing";
    return EXIT_SUCCESS;
}
