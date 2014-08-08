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
    void invalidStandardDeclaration_data();
    void invalidStandardDeclaration();
    void invalidDeclaration();
    void invalidRedeclaration();
    void removeIfUnused();
    void invalidMandatoryRouting();
    void invalidImmediateRouting();

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
    QTest::addColumn<bool>("delayedDeclaration");

    QTest::newRow("direct") << Exchange::Direct << false;
    QTest::newRow("direct-delayed") << Exchange::Direct << true;
    QTest::newRow("fanout") << Exchange::FanOut << false;
    QTest::newRow("fanout-delayed") << Exchange::FanOut << true;
    QTest::newRow("topic") << Exchange::Topic << false;
    QTest::newRow("topic-delayed") << Exchange::Topic << true;
    QTest::newRow("headers") << Exchange::Headers << false;
    QTest::newRow("headers-delayed") << Exchange::Headers << true;
}

void tst_QAMQPExchange::standardTypes()
{
    QFETCH(Exchange::ExchangeType, type);
    QFETCH(bool, delayedDeclaration);

    Exchange *exchange = client->createExchange("test");
    if (!delayedDeclaration)
        QVERIFY(waitForSignal(exchange, SIGNAL(opened())));

    exchange->declare(type);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    exchange->remove(Exchange::roForce);
    QVERIFY(waitForSignal(exchange, SIGNAL(removed())));
}

void tst_QAMQPExchange::invalidStandardDeclaration_data()
{
    QTest::addColumn<QString>("exchangeName");
    QTest::addColumn<Exchange::ExchangeType>("type");
    QTest::addColumn<QAMQP::Error>("error");

    QTest::newRow("amq.direct") << "amq.direct" << Exchange::Direct << QAMQP::PreconditionFailedError;
    QTest::newRow("amq.fanout") << "amq.fanout" << Exchange::FanOut << QAMQP::PreconditionFailedError;
    QTest::newRow("amq.headers") << "amq.headers" << Exchange::Headers << QAMQP::PreconditionFailedError;
    QTest::newRow("amq.match") << "amq.match" << Exchange::Headers << QAMQP::PreconditionFailedError;
    QTest::newRow("amq.topic") << "amq.topic" << Exchange::Topic << QAMQP::PreconditionFailedError;
    QTest::newRow("amq.reserved") << "amq.reserved" << Exchange::Direct << QAMQP::AccessRefusedError;
}

void tst_QAMQPExchange::invalidStandardDeclaration()
{
    QFETCH(QString, exchangeName);
    QFETCH(Exchange::ExchangeType, type);
    QFETCH(QAMQP::Error, error);

    Exchange *exchange = client->createExchange(exchangeName);
    exchange->declare(type);
    QVERIFY(waitForSignal(exchange, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(exchange->error(), error);
}

void tst_QAMQPExchange::invalidDeclaration()
{
    Exchange *exchange = client->createExchange("test-invalid-declaration");
    exchange->declare("invalidExchangeType");
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::CommandInvalidError);
}

void tst_QAMQPExchange::invalidRedeclaration()
{
    Exchange *exchange = client->createExchange("test-invalid-redeclaration");
    exchange->declare(Exchange::Direct);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));

    Exchange *redeclared = client->createExchange("test-invalid-redeclaration");
    redeclared->declare(Exchange::FanOut);
    QVERIFY(waitForSignal(redeclared, SIGNAL(error(QAMQP::Error))));

    // this is per spec:
    // QCOMPARE(redeclared->error(), QAMQP::NotAllowedError);

    // this is for rabbitmq:
    QCOMPARE(redeclared->error(), QAMQP::PreconditionFailedError);

    // cleanup
    exchange->remove();
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

void tst_QAMQPExchange::invalidMandatoryRouting()
{
    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("some message", "unroutable-key", MessageProperties(), Exchange::poMandatory);
    QVERIFY(waitForSignal(defaultExchange, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(defaultExchange->error(), QAMQP::UnroutableKey);
}

void tst_QAMQPExchange::invalidImmediateRouting()
{
    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("some message", "unroutable-key", MessageProperties(), Exchange::poImmediate);
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::NotImplementedError);
}

QTEST_MAIN(tst_QAMQPExchange)
#include "tst_qamqpexchange.moc"
