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
    void invalidDeclaration_data();
    void invalidDeclaration();
    void invalidBind();
    void unnamed();
    void exclusiveAccess();
    void exclusiveRemoval();
    void notFound();
    void remove();
    void removeIfUnused();
    void removeIfEmpty();
    void unbind();
    void purge();
    void canOnlyStartConsumingOnce();
    void cancel();
    void invalidCancelBecauseNotConsuming();
    void invalidCancelBecauseInvalidConsumerTag();
    void getEmpty();
    void get();
    void verifyContentEncodingIssue33();
    void defineQos();
    void invalidQos();
    void qos();
    void invalidRoutingKey();

private:
    void declareQueueAndVerifyConsuming(Queue *queue);
    QScopedPointer<Client> client;

};

void tst_QAMQPQueue::declareQueueAndVerifyConsuming(Queue *queue)
{
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->consume());
    QSignalSpy spy(queue, SIGNAL(consuming(QString)));
    QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));
    QVERIFY(queue->isConsuming());
    QVERIFY(!spy.isEmpty());
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), queue->consumerTag());
}

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
    declareQueueAndVerifyConsuming(queue);

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-default-exchange");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
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
    declareQueueAndVerifyConsuming(queue);

    queue->bind(exchange, routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    Exchange *defaultExchange = client->createExchange(exchange);
    defaultExchange->publish("test message", routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QCOMPARE(queue->dequeue().payload(), QByteArray("test message"));
}

void tst_QAMQPQueue::invalidDeclaration_data()
{
    QTest::addColumn<QString>("queueName");
    QTest::addColumn<QAMQP::Error>("error");

    QTest::newRow("amq.direct") << "amq.direct" <<  QAMQP::AccessRefusedError;
    QTest::newRow("amq.fanout") << "amq.fanout" << QAMQP::AccessRefusedError;
    QTest::newRow("amq.headers") << "amq.headers" << QAMQP::AccessRefusedError;
    QTest::newRow("amq.match") << "amq.match" << QAMQP::AccessRefusedError;
    QTest::newRow("amq.topic") << "amq.topic" << QAMQP::AccessRefusedError;
    QTest::newRow("amq.reserved") << "amq.reserved" << QAMQP::AccessRefusedError;
}

void tst_QAMQPQueue::invalidDeclaration()
{
    QFETCH(QString, queueName);
    QFETCH(QAMQP::Error, error);

    Queue *queue = client->createQueue(queueName);
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), error);
}

void tst_QAMQPQueue::invalidBind()
{
    Queue *queue = client->createQueue("test-invalid-bind");
    declareQueueAndVerifyConsuming(queue);

    queue->bind("non-existant-exchange", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::NotFoundError);
}

void tst_QAMQPQueue::unnamed()
{
    Queue *queue = client->createQueue();
    declareQueueAndVerifyConsuming(queue);
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
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(passiveQueue->error(), QAMQP::ResourceLockedError);

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
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(passiveQueue->error(), QAMQP::NotFoundError);
    secondClient.disconnectFromHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
}

void tst_QAMQPQueue::notFound()
{
    Queue *queue = client->createQueue("test-not-found");
    queue->declare(Queue::Passive);
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::NotFoundError);
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
    declareQueueAndVerifyConsuming(queue);

    queue->remove(Queue::roIfUnused);
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::PreconditionFailedError);
    QVERIFY(!queue->errorString().isEmpty());
}

void tst_QAMQPQueue::removeIfEmpty()
{
    // declare the queue and send messages to it
    Queue *queue = client->createQueue("test-remove-if-empty");
    queue->declare(Queue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-remove-if-empty");

    // create a second client and try to delete the queue
    {
        Client secondClient;
        secondClient.connectToHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
        Queue *testDeleteQueue = secondClient.createQueue("test-remove-if-empty");
        testDeleteQueue->declare(Queue::Passive);
        QVERIFY(waitForSignal(testDeleteQueue, SIGNAL(declared())));

        testDeleteQueue->remove(Queue::roIfEmpty);
        QVERIFY(waitForSignal(testDeleteQueue, SIGNAL(error(QAMQP::Error))));
        QCOMPARE(testDeleteQueue->error(), QAMQP::PreconditionFailedError);
        QVERIFY(!testDeleteQueue->errorString().isEmpty());

        secondClient.disconnectFromHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
    }

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::unbind()
{
    Queue *queue = client->createQueue("test-unbind");
    declareQueueAndVerifyConsuming(queue);

    queue->bind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->unbind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));
}

void tst_QAMQPQueue::purge()
{
    Queue *queue = client->createQueue("test-purge");
    queue->declare(Queue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-purge");
    defaultExchange->publish("second message", "test-purge");
    defaultExchange->publish("third message", "test-purge");

    // create second client to listen to messages and attempt purge
    {
        Client secondClient;
        secondClient.connectToHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
        Queue *testPurgeQueue = secondClient.createQueue("test-purge");
        testPurgeQueue->declare(Queue::Passive);
        QVERIFY(waitForSignal(testPurgeQueue, SIGNAL(declared())));

        QSignalSpy spy(testPurgeQueue, SIGNAL(purged(int)));
        testPurgeQueue->purge();
        QVERIFY(waitForSignal(testPurgeQueue, SIGNAL(purged(int))));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(testPurgeQueue->size(), 0);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toInt(), 3);

        secondClient.disconnectFromHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
    }

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::canOnlyStartConsumingOnce()
{
    Queue *queue = client->createQueue("test-single-consumer");
    declareQueueAndVerifyConsuming(queue);
    QCOMPARE(queue->consume(), false);
}

void tst_QAMQPQueue::cancel()
{
    Queue *queue = client->createQueue("test-cancel");
    declareQueueAndVerifyConsuming(queue);

    QString consumerTag = queue->consumerTag();
    QSignalSpy cancelSpy(queue, SIGNAL(cancelled(QString)));
    QVERIFY(queue->cancel());
    QVERIFY(waitForSignal(queue, SIGNAL(cancelled(QString))));
    QVERIFY(!cancelSpy.isEmpty());
    QList<QVariant> arguments = cancelSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), consumerTag);
}

void tst_QAMQPQueue::invalidCancelBecauseNotConsuming()
{
    Queue *queue = client->createQueue("test-invalid-cancel-because-not-consuming");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QCOMPARE(queue->cancel(), false);

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::invalidCancelBecauseInvalidConsumerTag()
{
    Queue *queue = client->createQueue("test-invalid-cancel-because-invalid-consumer-tag");
    declareQueueAndVerifyConsuming(queue);
    queue->setConsumerTag(QString());
    QCOMPARE(queue->cancel(), false);
}

void tst_QAMQPQueue::getEmpty()
{
    Queue *queue = client->createQueue("test-get-empty");
    declareQueueAndVerifyConsuming(queue);

    queue->get();
    QVERIFY(waitForSignal(queue, SIGNAL(empty())));
}

void tst_QAMQPQueue::get()
{
    Queue *queue = client->createQueue("test-get");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));

    const int messageCount = 50;
    Exchange *defaultExchange = client->createExchange();
    for (int i = 0; i < messageCount; ++i) {
        QString expected = QString("message %1").arg(i);
        defaultExchange->publish(expected, "test-get");
    }

    // wait for messages to be delivered
    QTest::qWait(25);

    for (int i = 0; i < messageCount; ++i) {
        QString expected = QString("message %1").arg(i);
        queue->get(false);
        QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
        Message message = queue->dequeue();
        QCOMPARE(message.payload(), expected.toUtf8());
        queue->ack(message);
    }

    queue->get(false);
    QVERIFY(waitForSignal(queue, SIGNAL(empty())));

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::verifyContentEncodingIssue33()
{
    Queue *queue = client->createQueue("test-issue-33");
    declareQueueAndVerifyConsuming(queue);

    Exchange *defaultExchange = client->createExchange();
    MessageProperties properties;
    properties.insert(Frame::Content::cpContentEncoding, "fakeContentEncoding");
    defaultExchange->publish("some data", "test-issue-33", properties);

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
    QString contentType =
        message.properties().value(Frame::Content::cpContentEncoding).toString();
    QCOMPARE(contentType, QLatin1String("fakeContentEncoding"));
}

void tst_QAMQPQueue::defineQos()
{
    Queue *queue = client->createQueue("test-define-qos");
    declareQueueAndVerifyConsuming(queue);

    queue->qos(10);
    QVERIFY(waitForSignal(queue, SIGNAL(qosDefined())));
    QCOMPARE(queue->prefetchCount(), qint16(10));
    QCOMPARE(queue->prefetchSize(), 0);

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::invalidQos()
{
    Queue *queue = client->createQueue("test-invalid-define-qos");
    declareQueueAndVerifyConsuming(queue);

    queue->qos(10, 10);
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::NotImplementedError);
}

void tst_QAMQPQueue::qos()
{
    Queue *queue = client->createQueue("test-qos");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));

    queue->qos(1);
    QVERIFY(waitForSignal(queue, SIGNAL(qosDefined())));
    QCOMPARE(queue->prefetchCount(), qint16(1));
    QCOMPARE(queue->prefetchSize(), 0);

    // load up the queue
    const int messageCount = 10;
    Exchange *defaultExchange = client->createExchange();
    for (int i = 0; i < messageCount; ++i) {
        QString message = QString("message %1").arg(i);
        defaultExchange->publish(message, "test-qos");
    }

    QTest::qWait(100);

    // begin consuming, one at a time
    QVERIFY(queue->consume());
    QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));

    int messageReceivedCount = 0;
    while (!queue->isEmpty()) {
        QString expected = QString("message %1").arg(messageReceivedCount);
        Message message = queue->dequeue();
        QCOMPARE(message.payload(), expected.toUtf8());
        queue->ack(message);
        messageReceivedCount++;

        if (messageReceivedCount < messageCount)
            QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    }

    QCOMPARE(messageReceivedCount, messageCount);
}

void tst_QAMQPQueue::invalidRoutingKey()
{
    QString routingKey = QString("%1").arg('1', 256, QLatin1Char('0'));
    Queue *queue = client->createQueue(routingKey);
    queue->declare();
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::FrameError);
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
