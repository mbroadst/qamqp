#include <QtTest/QtTest>
#include "signalspy.h"

#include "amqp_client.h"
#include "amqp_exchange.h"

using namespace QAMQP;
class tst_QAMQPExchange : public QObject
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
    SignalSpy connectSpy(&client, SIGNAL(connected()));
    client.connectToHost();
    QVERIFY(connectSpy.wait());

    Exchange *exchange = client.createExchange("test");
    SignalSpy declareSpy(exchange, SIGNAL(declared()));
    exchange->declare(type);
    QVERIFY(declareSpy.wait());

    SignalSpy removeSpy(exchange, SIGNAL(removed()));
    exchange->remove(false, false);
    QVERIFY(removeSpy.wait());
}

QTEST_MAIN(tst_QAMQPExchange)
#include "tst_qamqpexchange.moc"
