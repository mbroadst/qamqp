#include <QtTest/QtTest>
#include "amqp_testcase.h"

#include "amqp_client.h"
#include "amqp_queue.h"
#include "amqp_exchange.h"

using namespace QAMQP;
class tst_QAMQPQueue : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void defaultExchange();

};

void tst_QAMQPQueue::defaultExchange()
{
    Client client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));

    Queue *queue = client.createQueue("test-default-exchange");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    Exchange *defaultExchange = client.createExchange();
    defaultExchange->publish("test-default-exchange", "first message");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived(Queue*))));
    Message message = queue->getMessage();
    QCOMPARE(message.payload(), QByteArray("first message"));

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
