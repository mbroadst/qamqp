#include <QScopedPointer>

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
    void init();
    void cleanup();

    void defaultExchange();
    void standardExchanges_data();
    void standardExchanges();

    void unnamed();
    void exclusiveAccess();
    void exclusiveRemoval();

    void remove();
    void removeIfUnused();
    void unbind();

private:    // disabled
    void removeIfEmpty();

private:
    QScopedPointer<Client> client;

};

void tst_QAMQPQueue::init()
{
    client.reset(new Client);
    client->connectToHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(connected())));
}

void tst_QAMQPQueue::cleanup()
{
    if (client->isConnected()) {
        client->disconnectFromHost();
        QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));
    }
}

void tst_QAMQPQueue::defaultExchange()
{
    Queue *queue = client->createQueue("test-default-exchange");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("test-default-exchange", "first message");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->getMessage();
    QCOMPARE(message.payload(), QByteArray("first message"));
}

void tst_QAMQPQueue::standardExchanges_data()
{
    QTest::addColumn<QString>("exchange");
    QTest::newRow("amq.direct") << "amq.direct";
    QTest::newRow("amq.fanout") << "amq.fanout";
    QTest::newRow("amq.headers") << "amq.headers";
    QTest::newRow("amq.match") << "amq.match";
    QTest::newRow("amq.topic") << "amq.topic";
}

void tst_QAMQPQueue::standardExchanges()
{
    QFETCH(QString, exchange);

    QString queueName = QString("test-%1").arg(exchange);
    QString routingKey = QString("testRoutingKey-%1").arg(exchange);

    Queue *queue = client->createQueue(queueName);
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();   // required because AutoDelete will not delete if
                        // there was never a consumer

    queue->bind(exchange, routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    Exchange *defaultExchange = client->createExchange(exchange);
    defaultExchange->publish(routingKey, "test message");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QCOMPARE(queue->getMessage().payload(), QByteArray("test message"));
}

void tst_QAMQPQueue::unnamed()
{
    Queue *queue = client->createQueue();
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    QVERIFY(!queue->name().isEmpty());
}

void tst_QAMQPQueue::exclusiveAccess()
{
    Queue *queue = client->createQueue("test-exclusive-queue");
    queue->declare(Queue::Exclusive);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));

    Client secondClient;
    secondClient.connectToHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
    Queue *passiveQueue = secondClient.createQueue("test-exclusive-queue");
    passiveQueue->declare(Queue::Passive);
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(ChannelError))));
    QCOMPARE(passiveQueue->error(), Channel::ResourceLockedError);

    secondClient.disconnectFromHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
}

void tst_QAMQPQueue::exclusiveRemoval()
{
    Queue *queue = client->createQueue("test-exclusive-queue");
    queue->declare(Queue::Exclusive);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    client.data()->disconnectFromHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));

    // create a new client and try to access the queue that should
    // no longer exist
    Client secondClient;
    secondClient.connectToHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
    Queue *passiveQueue = secondClient.createQueue("test-exclusive-queue");
    passiveQueue->declare(Queue::Passive);
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(ChannelError))));
    QCOMPARE(passiveQueue->error(), Channel::NotFoundError);
    secondClient.disconnectFromHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
}

void tst_QAMQPQueue::remove()
{
    Queue *queue = client->createQueue("test-remove");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->remove(Queue::roIfEmpty|Queue::roIfUnused);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::removeIfUnused()
{
    Queue *queue = client->createQueue("test-remove-if-unused");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    queue->remove(Queue::roIfUnused);
    QVERIFY(waitForSignal(queue, SIGNAL(error(ChannelError))));
    QCOMPARE(queue->error(), Channel::PreconditionFailedError);
    QVERIFY(!queue->errorString().isEmpty());
}

void tst_QAMQPQueue::removeIfEmpty()
{
    // NOTE: this will work once I refactor messages to easily
    //       add propertis for e.g. persistence

    Queue *queue = client->createQueue("test-remove-if-empty");
    queue->declare(Queue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("test-remove-if-empty", "first message");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));

    queue->remove(Queue::roIfEmpty);
    QVERIFY(waitForSignal(queue, SIGNAL(error(ChannelError))));
    QCOMPARE(queue->error(), Channel::PreconditionFailedError);
    QVERIFY(!queue->errorString().isEmpty());
}

void tst_QAMQPQueue::unbind()
{
    Queue *queue = client->createQueue("test-unbind");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();   // required because AutoDelete will not delete if
                        // there was never a consumer

    queue->bind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->unbind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
