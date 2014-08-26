#include <float.h>

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
    void bindUnbind();
    void delayedBind();
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
    void tableFieldDataTypes();
    void messageProperties();

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
    declareQueueAndVerifyConsuming(queue);

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-default-exchange");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
    verifyStandardMessageHeaders(message, "test-default-exchange");
    QCOMPARE(message.payload(), QByteArray("first message"));
}

void tst_QAMQPQueue::standardExchanges_data()
{
    QTest::addColumn<QString>("exchange");
    QTest::addColumn<bool>("delayedDeclaration");

    QTest::newRow("amq.direct") << "amq.direct" << false;
    QTest::newRow("amq.direct-delayed") << "amq.direct" << true;
    QTest::newRow("amq.fanout") << "amq.fanout" << false;
    QTest::newRow("amq.fanout-delayed") << "amq.fanout" << true;
    QTest::newRow("amq.headers") << "amq.headers" << false;
    QTest::newRow("amq.headers-delayed") << "amq.headers" << true;
    QTest::newRow("amq.match") << "amq.match" << false;
    QTest::newRow("amq.match-delayed") << "amq.match" << true;
    QTest::newRow("amq.topic") << "amq.topic" << false;
    QTest::newRow("amq.topic-delayed") << "amq.topic" << true;
}

void tst_QAMQPQueue::standardExchanges()
{
    QFETCH(QString, exchange);
    QFETCH(bool, delayedDeclaration);

    QString queueName = QString("test-%1").arg(exchange);
    QString routingKey = QString("testRoutingKey-%1").arg(exchange);

    Queue *queue = client->createQueue(queueName);
    if (!delayedDeclaration)
        QVERIFY(waitForSignal(queue, SIGNAL(opened())));
    declareQueueAndVerifyConsuming(queue);

    queue->bind(exchange, routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    Exchange *defaultExchange = client->createExchange(exchange);
    defaultExchange->publish("test message", routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
    verifyStandardMessageHeaders(message, routingKey, exchange);
    QCOMPARE(message.payload(), QByteArray("test message"));
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
    QVERIFY(queue->options() & Queue::Exclusive);

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
    QVERIFY(queue->options() & Queue::Exclusive);
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
    QVERIFY(queue->options() & Queue::Durable);

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
        QVERIFY(testDeleteQueue->options() & Queue::Passive);

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

void tst_QAMQPQueue::bindUnbind()
{
    Queue *queue = client->createQueue("test-bind-unbind");
    declareQueueAndVerifyConsuming(queue);

    queue->bind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->unbind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));

    Exchange *amqTopic = client->createExchange("amq.topic");
    amqTopic->declare(Exchange::Direct, Exchange::Passive);
    QVERIFY(waitForSignal(amqTopic, SIGNAL(declared())));
    queue->bind(amqTopic, "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->unbind(amqTopic, "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));
}

void tst_QAMQPQueue::delayedBind()
{
    client->disconnectFromHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));
    Queue *queue = client->createQueue("test-delayed-bind");
    queue->declare();
    queue->bind("amq.topic", "routingKey");

    client->connectToHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(connected())));
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    // clean up queue
    queue->remove(Queue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::purge()
{
    Queue *queue = client->createQueue("test-purge");
    queue->declare(Queue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->options() & Queue::Durable);

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
        QVERIFY(testPurgeQueue->options() & Queue::Passive);

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

    const int messageCount = 200;
    Exchange *defaultExchange = client->createExchange();
    for (int i = 0; i < messageCount; ++i) {
        QString expected = QString("message %1").arg(i);
        defaultExchange->publish(expected, "test-get");
    }

    for (int i = 0; i < messageCount; ++i) {
        QString expected = QString("message %1").arg(i);
        queue->get(false);
        if (!waitForSignal(queue, SIGNAL(messageReceived()))) {
            // NOTE: this is here instead of waiting for messages to be
            //       available with a sleep above. It makes the test a little
            //       longer if there's a miss, look into a proper fix in the future
            i--;
            continue;
        }

        Message message = queue->dequeue();
        verifyStandardMessageHeaders(message, "test-get");
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
    Message::PropertyHash properties;
    properties.insert(Message::ContentEncoding, "fakeContentEncoding");
    defaultExchange->publish("some data", "test-issue-33", properties);

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
    verifyStandardMessageHeaders(message, "test-issue-33");
    QVERIFY(message.hasProperty(Message::ContentEncoding));
    QString contentType = message.property(Message::ContentEncoding).toString();
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
    QVERIFY(queue->consume());
    QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));

    // load up the queue
    const int messageCount = 10;
    Exchange *defaultExchange = client->createExchange();
    for (int i = 0; i < messageCount; ++i) {
        QString message = QString("message %1").arg(i);
        defaultExchange->publish(message, "test-qos");
    }

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    int messageReceivedCount = 0;
    while (!queue->isEmpty()) {
        QString expected = QString("message %1").arg(messageReceivedCount);
        Message message = queue->dequeue();
        verifyStandardMessageHeaders(message, "test-qos");
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

void tst_QAMQPQueue::tableFieldDataTypes()
{
    Queue *queue = client->createQueue("test-table-field-data-types");
    declareQueueAndVerifyConsuming(queue);

    QAMQP::Decimal decimal;
    decimal.scale = 2;
    decimal.value = 12345;
    QVariant decimalVariant = QVariant::fromValue<QAMQP::Decimal>(decimal);

    Table nestedTable;
    nestedTable.insert("boolean", true);
    nestedTable.insert("long-int", qint32(-65536));

    QVariantList array;
    array.append(true);
    array.append(qint32(-65536));

    QDateTime timestamp = QDateTime::currentDateTime();

    Table headers;
    headers.insert("boolean", true);
    headers.insert("short-short-int", qint8(-15));
    headers.insert("short-short-uint", quint8(15));
    headers.insert("short-int", qint16(-256));
    headers.insert("short-uint", QVariant::fromValue(quint16(256)));
    headers.insert("long-int", qint32(-65536));
    headers.insert("long-uint", quint32(65536));
    headers.insert("long-long-int", qint64(-2147483648));
    headers.insert("long-long-uint", quint64(2147483648));
    headers.insert("float", 230.7);
    headers.insert("double", double(FLT_MAX));
    headers.insert("decimal-value", decimalVariant);
    headers.insert("short-string", QLatin1String("test"));
    headers.insert("long-string", QLatin1String("test"));
    headers.insert("timestamp", timestamp);
    headers.insert("nested-table", nestedTable);
    headers.insert("array", array);
    headers.insert("bytes", QByteArray("abcdefg1234567"));

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("dummy", "test-table-field-data-types", "text.plain", headers);

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();

    QCOMPARE(message.header("boolean").toBool(), true);
    QCOMPARE(qint8(message.header("short-short-int").toInt()), qint8(-15));
    QCOMPARE(quint8(message.header("short-short-uint").toUInt()), quint8(15));
    QCOMPARE(qint16(message.header("short-int").toInt()), qint16(-256));
    QCOMPARE(quint16(message.header("short-uint").toUInt()), quint16(256));
    QCOMPARE(qint32(message.header("long-int").toInt()), qint32(-65536));
    QCOMPARE(quint32(message.header("long-uint").toUInt()), quint32(65536));
    QCOMPARE(qint64(message.header("long-long-int").toLongLong()), qint64(-2147483648));
    QCOMPARE(quint64(message.header("long-long-uint").toLongLong()), quint64(2147483648));
    QCOMPARE(message.header("float").toFloat(), float(230.7));
    QCOMPARE(message.header("double").toDouble(), double(FLT_MAX));
    QCOMPARE(message.header("short-string").toString(), QLatin1String("test"));
    QCOMPARE(message.header("long-string").toString(), QLatin1String("test"));
    QCOMPARE(message.header("timestamp").toDateTime(), timestamp);
    QCOMPARE(message.header("bytes").toByteArray(), QByteArray("abcdefg1234567"));

    QVERIFY(message.hasHeader("nested-table"));
    Table compareTable(message.header("nested-table").toHash());
    foreach (QString key, nestedTable.keys()) {
        QVERIFY(compareTable.contains(key));
        QCOMPARE(nestedTable.value(key), compareTable.value(key));
    }

    QVERIFY(message.hasHeader("array"));
    QVariantList compareArray = message.header("array").toList();
    QCOMPARE(array, compareArray);

    QAMQP::Decimal receivedDecimal = message.header("decimal-value").value<QAMQP::Decimal>();
    QCOMPARE(receivedDecimal.scale, qint8(2));
    QCOMPARE(receivedDecimal.value, quint32(12345));
}

void tst_QAMQPQueue::messageProperties()
{
    Queue *queue = client->createQueue("test-message-properties");
    declareQueueAndVerifyConsuming(queue);

    QDateTime timestamp = QDateTime::currentDateTime();
    Message::PropertyHash properties;
    properties.insert(Message::ContentType, "some-content-type");
    properties.insert(Message::ContentEncoding, "some-content-encoding");
    properties.insert(Message::DeliveryMode, 2);
    properties.insert(Message::Priority, 5);
    properties.insert(Message::CorrelationId, 42);
    properties.insert(Message::ReplyTo, "another-queue");
    properties.insert(Message::MessageId, "some-message-id");
    properties.insert(Message::Expiration, "60000");
    properties.insert(Message::Timestamp, timestamp);
    properties.insert(Message::Type, "some-message-type");
    properties.insert(Message::UserId, "guest");
    properties.insert(Message::AppId, "some-app-id");
    properties.insert(Message::ClusterID, "some-cluster-id");

    Exchange *defaultExchange = client->createExchange();
    defaultExchange->publish("dummy", "test-message-properties", properties);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();

    QCOMPARE(message.property(Message::ContentType).toString(), QLatin1String("some-content-type"));
    QCOMPARE(message.property(Message::ContentEncoding).toString(), QLatin1String("some-content-encoding"));
    QCOMPARE(message.property(Message::DeliveryMode).toInt(), 2);
    QCOMPARE(message.property(Message::Priority).toInt(), 5);
    QCOMPARE(message.property(Message::CorrelationId).toInt(), 42);
    QCOMPARE(message.property(Message::ReplyTo).toString(), QLatin1String("another-queue"));
    QCOMPARE(message.property(Message::MessageId).toString(), QLatin1String("some-message-id"));
    QCOMPARE(message.property(Message::Expiration).toString(), QLatin1String("60000"));
    QCOMPARE(message.property(Message::Timestamp).toDateTime(), timestamp);
    QCOMPARE(message.property(Message::Type).toString(), QLatin1String("some-message-type"));
    QCOMPARE(message.property(Message::UserId).toString(), QLatin1String("guest"));
    QCOMPARE(message.property(Message::AppId).toString(), QLatin1String("some-app-id"));
    QCOMPARE(message.property(Message::ClusterID).toString(), QLatin1String("some-cluster-id"));
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
