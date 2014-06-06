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

    void remove();
    void removeIfUnused();

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
    client->disconnectFromHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));
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
    QCOMPARE(queue->error(), Channel::PreconditionFailed);
}

void tst_QAMQPQueue::removeIfEmpty()
{
    // NOTE: this will work once I refactor messages to easily
    //       add propertis for e.g. persistence

    Queue *queue = client->createQueue();
    queue->declare("test-remove-if-empty", Queue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->consume();

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("test-remove-if-empty", "first message");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));

    queue->remove(Queue::roIfEmpty);
    QVERIFY(waitForSignal(queue, SIGNAL(error(ChannelError))));
    QCOMPARE(queue->error(), Channel::PreconditionFailed);
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
