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
    void init();
    void cleanup();

    void standardTypes_data();
    void standardTypes();
    void removeIfUnused();

private:
    QScopedPointer<Client> client;

};

void tst_QAMQPExchange::init()
{
    client.reset(new Client);
    client->connectToHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(connected())));
}

void tst_QAMQPExchange::cleanup()
{
    if (client->isConnected()) {
        client->disconnectFromHost();
        QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));
    }
}

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

    Exchange *exchange = client->createExchange("test");
    exchange->declare(type);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    exchange->remove(Exchange::roForce);
    QVERIFY(waitForSignal(exchange, SIGNAL(removed())));
}

void tst_QAMQPExchange::removeIfUnused()
{
    Exchange *exchange = client->createExchange("test-if-unused-exchange");
    exchange->declare(Exchange::Direct, Exchange::AutoDelete);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));

    Queue *queue = client->createQueue("test-if-unused-queue");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->bind("test-if-unused-exchange", "testRoutingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    exchange->remove(Exchange::roIfUnused);
    QVERIFY(waitForSignal(exchange, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(exchange->error(), QAMQP::PreconditionFailedError);
    QVERIFY(!exchange->errorString().isEmpty());

    // cleanup
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

QTEST_MAIN(tst_QAMQPExchange)
#include "tst_qamqpexchange.moc"
