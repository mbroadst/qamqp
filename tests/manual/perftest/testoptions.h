#ifndef TESTOPTIONS_H
#define TESTOPTIONS_H

#include <QString>

#if QT_VERSION < 0x050000
#include "qcommandlineparser/qcommandlineparser.h"
#else
#include <QCommandLineParser>
#endif

struct TestOptions
{
    explicit TestOptions(const QCommandLineParser &parser);

    qreal confirm;
    int consumerCount;
    int producerCount;
    int consumerTxSize;
    int producerTxSize;
    int channelPrefetch;
    int consumerPrefetch;
    int minMsgSize;

    int timeLimit;
    float rateLimit;
    int producerMsgCount;
    int consumerMsgCount;

    QString exchangeName;
    QString exchangeType;
    QString queueName;
    QString routingKey;

    int multiAckEvery;
    bool autoAck;
    bool autoDelete;

    bool predeclared;
};

#endif
