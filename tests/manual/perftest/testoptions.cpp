#include "testoptions.h"

TestOptions::TestOptions(const QCommandLineParser &parser)
    : confirm(-1),
      consumerCount(1),
      producerCount(1),
      consumerTxSize(0),
      producerTxSize(0),
      channelPrefetch(0),
      consumerPrefetch(0),
      minMsgSize(0),
      timeLimit(0),
      rateLimit(0),
      producerMsgCount(0),
      consumerMsgCount(0),
      exchangeName("direct"),
      exchangeType("direct"),
      multiAckEvery(0),
      autoAck(true),
      autoDelete(false),
      predeclared(false)
{
    if (parser.isSet("t"))
        exchangeType = parser.value("t");
    if (parser.isSet("e"))
        exchangeName = parser.value("e");
    if (parser.isSet("u"))
        queueName = parser.value("u");
    if (parser.isSet("k"))
        routingKey = parser.value("k");
    if (parser.isSet("r"))
        rateLimit = parser.value("r").toFloat();
    if (parser.isSet("x"))
        producerCount = parser.value("x").toInt();
    if (parser.isSet("y"))
        consumerCount = parser.value("y").toInt();
    if (parser.isSet("m"))
        producerTxSize = parser.value("m").toInt();
    if (parser.isSet("n"))
        consumerTxSize = parser.value("n").toInt();
    if (parser.isSet("c"))
        confirm = parser.value("c").toInt();
    autoAck = parser.isSet("a");
    if (parser.isSet("A"))
        multiAckEvery = parser.value("A").toInt();
    if (parser.isSet("Q"))
        channelPrefetch = parser.value("Q").toInt();
    if (parser.isSet("q"))
        consumerPrefetch = parser.value("q").toInt();
    if (parser.isSet("s"))
        minMsgSize = parser.value("s").toInt();
    if (parser.isSet("z"))
        timeLimit = parser.value("z").toInt();
    if (parser.isSet("C"))
        producerMsgCount = parser.value("C").toInt();
    if (parser.isSet("D"))
        consumerMsgCount = parser.value("D").toInt();
    predeclared = parser.isSet("p");
}
