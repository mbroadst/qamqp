#include <QtTest/QtTest>

#include "signalspy.h"
#include "amqp_testcase.h"

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"

using namespace QAMQP;
class tst_QAMQPExchange : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void standardTypes_data();
    void standardTypes();

};

void tst_QAMQPExchange::standardTypes_data()
{
    QTest::addColumn<Exchange::ExchangeType>("type");
    QTest::newRow("direct") << Exchange::Direct;
    QTest::newRow("fanout") << Exchange::FanOut;
    QTest::newRow("topic") << Exchange::Topic;
    QTest::newRow("headers") << Exchange::Headers;
}

void tst_QAMQPExchange::standardTypes()
{
    QFETCH(Exchange::ExchangeType, type);

    Client client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));

    Exchange *exchange = client.createExchange("test");
    exchange->declare(type);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    exchange->remove(false, false);
    QVERIFY(waitForSignal(exchange, SIGNAL(removed())));

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

QTEST_MAIN(tst_QAMQPExchange)
#include "tst_qamqpexchange.moc"
