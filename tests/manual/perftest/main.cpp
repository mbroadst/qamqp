#include <QCoreApplication>
#include <QThreadPool>
#include <QUuid>

#if QT_VERSION < 0x050000
#include "qcommandlineparser/qcommandlineparser.h"
#else
#include <QCommandLineParser>
#endif

#include "producer.h"
#include "testoptions.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();

    parser.addOption(QCommandLineOption(QStringList() << "t" << "type", "exchange type", "type", "direct"));
    parser.addOption(QCommandLineOption(QStringList() << "e" << "exchange", "exchange name", "name", ""));
    parser.addOption(QCommandLineOption(QStringList() << "u" << "queue", "queue name", "name", ""));
    parser.addOption(QCommandLineOption(QStringList() << "k" << "routingKey", "routing key", "key"));
    parser.addOption(QCommandLineOption(QStringList() << "i" << "interval", "sampling interval", "interval", "1"));
    parser.addOption(QCommandLineOption(QStringList() << "r" << "rate", "rate limit", "limit", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "x" << "producers", "producer count", "count", "1"));
    parser.addOption(QCommandLineOption(QStringList() << "y" << "consumers", "consumer count", "count", "1"));
    parser.addOption(QCommandLineOption(QStringList() << "m" << "ptxsize", "producer tx size", "size", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "n" << "ctxsize", "consumer tx size", "size", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "c" << "confirm", "max unconfirmed publishes"));
    parser.addOption(QCommandLineOption(QStringList() << "a" << "autoack", "auto ack"));
    parser.addOption(QCommandLineOption(QStringList() << "A" << "multiAckEvery", "multi ack every", "rate", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "Q" << "globalQos", "channel prefetch count", "count", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "q" << "qos", "consumer prefetch count", "count", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "s" << "size", "message size", "size", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "z" << "time", "time limit", "limit", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "C" << "pmessages", "producer message count", "count", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "D" << "cmessages", "consumer message count", "count", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "M" << "framemax", "frame max", "size", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "b" << "heartbeat", "heartbeat interval", "interval", "0"));
    parser.addOption(QCommandLineOption(QStringList() << "p" << "predeclared", "allow use of predeclared objects"));

    parser.process(app);
    TestOptions options(parser);
    QThreadPool::globalInstance()->setMaxThreadCount(options.producerCount + options.consumerCount);

    int samplingInterval = 0;
    if (parser.isSet("i"))
        samplingInterval = parser.value("i").toInt();

    int frameMax = 0;
    if (parser.isSet("M"))
        frameMax = parser.value("M").toInt();

    int heartbeat = 0;
    if (parser.isSet("b"))
        heartbeat = parser.value("b").toInt();

    QString id;
    if (options.routingKey.isEmpty())
        id = QUuid::createUuid().toString();
    else
        id = options.routingKey;

    for (int i = 0; i < options.producerCount; i++)
        QThreadPool::globalInstance()->start(new Producer(id, options));

    return app.exec();
}
