#include <float.h>

#include <QScopedPointer>

#include <QtTest/QtTest>
#include "qamqptestcase.h"
#include "signalspy.h"

#include "qamqpclient.h"
#include "qamqpqueue.h"
#include "qamqpexchange.h"

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
    void ensureConsumeOnlySentOnce();
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
    void emptyMessage();
    void cleanupOnDeletion();

private:
    QScopedPointer<QAmqpClient> client;

};

void tst_QAMQPQueue::init()
{
    client.reset(new QAmqpClient);
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
    QAmqpQueue *queue = client->createQueue("test-default-exchange");
    declareQueueAndVerifyConsuming(queue);

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-default-exchange");
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();
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

    QAmqpQueue *queue = client->createQueue(queueName);
    if (!delayedDeclaration)
        QVERIFY(waitForSignal(queue, SIGNAL(opened())));
    declareQueueAndVerifyConsuming(queue);

    queue->bind(exchange, routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    QAmqpExchange *defaultExchange = client->createExchange(exchange);
    defaultExchange->publish("test message", routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();
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

    QAmqpQueue *queue = client->createQueue(queueName);
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), error);
}

void tst_QAMQPQueue::invalidBind()
{
    QAmqpQueue *queue = client->createQueue("test-invalid-bind");
    declareQueueAndVerifyConsuming(queue);

    queue->bind("non-existant-exchange", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::NotFoundError);
}

void tst_QAMQPQueue::unnamed()
{
    QAmqpQueue *queue = client->createQueue();
    declareQueueAndVerifyConsuming(queue);
    QVERIFY(!queue->name().isEmpty());
}

void tst_QAMQPQueue::exclusiveAccess()
{
    QAmqpQueue *queue = client->createQueue("test-exclusive-queue");
    queue->declare(QAmqpQueue::Exclusive);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->options() & QAmqpQueue::Exclusive);

    QAmqpClient secondClient;
    secondClient.connectToHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
    QAmqpQueue *passiveQueue = secondClient.createQueue("test-exclusive-queue");
    passiveQueue->declare(QAmqpQueue::Passive);
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(passiveQueue->error(), QAMQP::ResourceLockedError);

    secondClient.disconnectFromHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
}

void tst_QAMQPQueue::exclusiveRemoval()
{
    QAmqpQueue *queue = client->createQueue("test-exclusive-queue");
    queue->declare(QAmqpQueue::Exclusive);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->options() & QAmqpQueue::Exclusive);
    client.data()->disconnectFromHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));

    // create a new client and try to access the queue that should
    // no longer exist
    QAmqpClient secondClient;
    secondClient.connectToHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
    QAmqpQueue *passiveQueue = secondClient.createQueue("test-exclusive-queue");
    passiveQueue->declare(QAmqpQueue::Passive);
    QVERIFY(waitForSignal(passiveQueue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(passiveQueue->error(), QAMQP::NotFoundError);
    secondClient.disconnectFromHost();
    QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
}

void tst_QAMQPQueue::notFound()
{
    QAmqpQueue *queue = client->createQueue("test-not-found");
    queue->declare(QAmqpQueue::Passive);
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::NotFoundError);
}

void tst_QAMQPQueue::remove()
{
    QAmqpQueue *queue = client->createQueue("test-remove");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->remove(QAmqpQueue::roIfEmpty|QAmqpQueue::roIfUnused);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::removeIfUnused()
{
    QAmqpQueue *queue = client->createQueue("test-remove-if-unused");
    declareQueueAndVerifyConsuming(queue);

    queue->remove(QAmqpQueue::roIfUnused);
    QVERIFY(waitForSignal(queue, SIGNAL(error(QAMQP::Error))));
    QCOMPARE(queue->error(), QAMQP::PreconditionFailedError);
    QVERIFY(!queue->errorString().isEmpty());
}

void tst_QAMQPQueue::removeIfEmpty()
{
    // declare the queue and send messages to it
    QAmqpQueue *queue = client->createQueue("test-remove-if-empty");
    queue->declare(QAmqpQueue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->options() & QAmqpQueue::Durable);

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-remove-if-empty");

    // create a second client and try to delete the queue
    {
        QAmqpClient secondClient;
        secondClient.connectToHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
        QAmqpQueue *testDeleteQueue = secondClient.createQueue("test-remove-if-empty");
        testDeleteQueue->declare(QAmqpQueue::Passive);
        QVERIFY(waitForSignal(testDeleteQueue, SIGNAL(declared())));
        QVERIFY(testDeleteQueue->options() & QAmqpQueue::Passive);

        testDeleteQueue->remove(QAmqpQueue::roIfEmpty);
        QVERIFY(waitForSignal(testDeleteQueue, SIGNAL(error(QAMQP::Error))));
        QCOMPARE(testDeleteQueue->error(), QAMQP::PreconditionFailedError);
        QVERIFY(!testDeleteQueue->errorString().isEmpty());

        secondClient.disconnectFromHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(disconnected())));
    }

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::bindUnbind()
{
    QAmqpQueue *queue = client->createQueue("test-bind-unbind");
    declareQueueAndVerifyConsuming(queue);

    queue->bind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->unbind("amq.topic", "routingKey");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));

    QAmqpExchange *amqTopic = client->createExchange("amq.topic");
    amqTopic->declare(QAmqpExchange::Direct, QAmqpExchange::Passive);
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
    QAmqpQueue *queue = client->createQueue("test-delayed-bind");
    queue->declare();
    queue->bind("amq.topic", "routingKey");

    client->connectToHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(connected())));
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::purge()
{
    QAmqpQueue *queue = client->createQueue("test-purge");
    queue->declare(QAmqpQueue::Durable);
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QVERIFY(queue->options() & QAmqpQueue::Durable);

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("first message", "test-purge");
    defaultExchange->publish("second message", "test-purge");
    defaultExchange->publish("third message", "test-purge");

    // create second client to listen to messages and attempt purge
    {
        QAmqpClient secondClient;
        secondClient.connectToHost();
        QVERIFY(waitForSignal(&secondClient, SIGNAL(connected())));
        QAmqpQueue *testPurgeQueue = secondClient.createQueue("test-purge");
        testPurgeQueue->declare(QAmqpQueue::Passive);
        QVERIFY(waitForSignal(testPurgeQueue, SIGNAL(declared())));
        QVERIFY(testPurgeQueue->options() & QAmqpQueue::Passive);

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
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::canOnlyStartConsumingOnce()
{
    QAmqpQueue *queue = client->createQueue("test-single-consumer");
    QSignalSpy spy(queue, SIGNAL(consuming(QString)));
    declareQueueAndVerifyConsuming(queue);
    QCOMPARE(queue->consume(), false);
}

void tst_QAMQPQueue::ensureConsumeOnlySentOnce()
{
    QAmqpQueue *queue = client->createQueue("test-single-consumer");
    QSignalSpy spy(queue, SIGNAL(consuming(QString)));
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));

    // try to consume twice
    QVERIFY(queue->consume());
    QCOMPARE(queue->consume(), false);
    QVERIFY(spy.wait());
    QCOMPARE(spy.size(), 1);

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::cancel()
{
    QAmqpQueue *queue = client->createQueue("test-cancel");
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
    QAmqpQueue *queue = client->createQueue("test-invalid-cancel-because-not-consuming");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    QCOMPARE(queue->cancel(), false);

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::invalidCancelBecauseInvalidConsumerTag()
{
    QAmqpQueue *queue = client->createQueue("test-invalid-cancel-because-invalid-consumer-tag");
    declareQueueAndVerifyConsuming(queue);
    queue->setConsumerTag(QString());
    QCOMPARE(queue->cancel(), false);
}

void tst_QAMQPQueue::getEmpty()
{
    QAmqpQueue *queue = client->createQueue("test-get-empty");
    declareQueueAndVerifyConsuming(queue);

    queue->get();
    QVERIFY(waitForSignal(queue, SIGNAL(empty())));
}

void tst_QAMQPQueue::get()
{
    QAmqpQueue *queue = client->createQueue("test-get");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));

    const int messageCount = 200;
    QAmqpExchange *defaultExchange = client->createExchange();
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

        QAmqpMessage message = queue->dequeue();
        verifyStandardMessageHeaders(message, "test-get");
        QCOMPARE(message.payload(), expected.toUtf8());
        queue->ack(message);
    }

    queue->get(false);
    QVERIFY(waitForSignal(queue, SIGNAL(empty())));

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::verifyContentEncodingIssue33()
{
    QAmqpQueue *queue = client->createQueue("test-issue-33");
    declareQueueAndVerifyConsuming(queue);

    QAmqpExchange *defaultExchange = client->createExchange();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::ContentEncoding, "fakeContentEncoding");
    defaultExchange->publish("some data", "test-issue-33", properties);

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();
    verifyStandardMessageHeaders(message, "test-issue-33");
    QVERIFY(message.hasProperty(QAmqpMessage::ContentEncoding));
    QString contentType = message.property(QAmqpMessage::ContentEncoding).toString();
    QCOMPARE(contentType, QLatin1String("fakeContentEncoding"));
}

void tst_QAMQPQueue::defineQos()
{
    QAmqpQueue *queue = client->createQueue("test-define-qos");
    declareQueueAndVerifyConsuming(queue);

    queue->qos(10);
    QVERIFY(waitForSignal(queue, SIGNAL(qosDefined())));
    QCOMPARE(queue->prefetchCount(), qint16(10));
    QCOMPARE(queue->prefetchSize(), 0);

    // clean up queue
    queue->remove(QAmqpQueue::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
}

void tst_QAMQPQueue::invalidQos()
{
    QAmqpQueue *queue = client->createQueue("test-invalid-define-qos");
    declareQueueAndVerifyConsuming(queue);

    queue->qos(10, 10);
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::NotImplementedError);
}

void tst_QAMQPQueue::qos()
{
    QAmqpQueue *queue = client->createQueue("test-qos");
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
    QAmqpExchange *defaultExchange = client->createExchange();
    for (int i = 0; i < messageCount; ++i) {
        QString message = QString("message %1").arg(i);
        defaultExchange->publish(message, "test-qos");
    }

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    int messageReceivedCount = 0;
    while (!queue->isEmpty()) {
        QString expected = QString("message %1").arg(messageReceivedCount);
        QAmqpMessage message = queue->dequeue();
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
    QAmqpQueue *queue = client->createQueue(routingKey);
    queue->declare();
    QVERIFY(waitForSignal(client.data(), SIGNAL(error(QAMQP::Error))));
    QCOMPARE(client->error(), QAMQP::FrameError);
}

void tst_QAMQPQueue::tableFieldDataTypes()
{
    QAmqpQueue *queue = client->createQueue("test-table-field-data-types");
    declareQueueAndVerifyConsuming(queue);

    QAMQP::Decimal decimal;
    decimal.scale = 2;
    decimal.value = 12345;
    QVariant decimalVariant = QVariant::fromValue<QAMQP::Decimal>(decimal);

    QAmqpTable nestedTable;
    nestedTable.insert("boolean", true);
    nestedTable.insert("long-int", qint32(-65536));

    QVariantList array;
    array.append(true);
    array.append(qint32(-65536));

    QDateTime timestamp = QDateTime::currentDateTime();

    QAmqpTable headers;
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

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("dummy", "test-table-field-data-types", "text.plain", headers);

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();

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
    QCOMPARE(message.header("bytes").toByteArray(), QByteArray("abcdefg1234567"));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    QCOMPARE(message.header("timestamp").toDateTime().toSecsSinceEpoch(),
             timestamp.toSecsSinceEpoch());
#else
    QCOMPARE(message.header("timestamp").toDateTime().toTime_t(), timestamp.toTime_t());
#endif

    QVERIFY(message.hasHeader("nested-table"));
    QAmqpTable compareTable(message.header("nested-table").toHash());
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
    QAmqpQueue *queue = client->createQueue("test-message-properties");
    declareQueueAndVerifyConsuming(queue);

    QDateTime timestamp = QDateTime::currentDateTime();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::ContentType, "some-content-type");
    properties.insert(QAmqpMessage::ContentEncoding, "some-content-encoding");
    properties.insert(QAmqpMessage::DeliveryMode, 2);
    properties.insert(QAmqpMessage::Priority, 5);
    properties.insert(QAmqpMessage::CorrelationId, 42);
    properties.insert(QAmqpMessage::ReplyTo, "another-queue");
    properties.insert(QAmqpMessage::MessageId, "some-message-id");
    properties.insert(QAmqpMessage::Expiration, "60000");
    properties.insert(QAmqpMessage::Timestamp, timestamp);
    properties.insert(QAmqpMessage::Type, "some-message-type");
    properties.insert(QAmqpMessage::UserId, "guest");
    properties.insert(QAmqpMessage::AppId, "some-app-id");
    properties.insert(QAmqpMessage::ClusterID, "some-cluster-id");

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("dummy", "test-message-properties", properties);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();

    QCOMPARE(message.property(QAmqpMessage::ContentType).toString(), QLatin1String("some-content-type"));
    QCOMPARE(message.property(QAmqpMessage::ContentEncoding).toString(), QLatin1String("some-content-encoding"));
    QCOMPARE(message.property(QAmqpMessage::DeliveryMode).toInt(), 2);
    QCOMPARE(message.property(QAmqpMessage::Priority).toInt(), 5);
    QCOMPARE(message.property(QAmqpMessage::CorrelationId).toInt(), 42);
    QCOMPARE(message.property(QAmqpMessage::ReplyTo).toString(), QLatin1String("another-queue"));
    QCOMPARE(message.property(QAmqpMessage::MessageId).toString(), QLatin1String("some-message-id"));
    QCOMPARE(message.property(QAmqpMessage::Expiration).toString(), QLatin1String("60000"));
    QCOMPARE(message.property(QAmqpMessage::Type).toString(), QLatin1String("some-message-type"));
    QCOMPARE(message.property(QAmqpMessage::UserId).toString(), QLatin1String("guest"));
    QCOMPARE(message.property(QAmqpMessage::AppId).toString(), QLatin1String("some-app-id"));
    QCOMPARE(message.property(QAmqpMessage::ClusterID).toString(), QLatin1String("some-cluster-id"));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    QCOMPARE(message.property(QAmqpMessage::Timestamp).toDateTime().toSecsSinceEpoch(),
             timestamp.toSecsSinceEpoch());
#else
    QCOMPARE(message.property(QAmqpMessage::Timestamp).toDateTime().toTime_t(),
             timestamp.toTime_t());
#endif
}

void tst_QAMQPQueue::emptyMessage()
{
    QAmqpQueue *queue = client->createQueue("test-issue-43");
    declareQueueAndVerifyConsuming(queue);

    QAmqpExchange *defaultExchange = client->createExchange();
    defaultExchange->publish("", "test-issue-43");

    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    QAmqpMessage message = queue->dequeue();
    verifyStandardMessageHeaders(message, "test-issue-43");
    QVERIFY(message.payload().isEmpty());
}

void tst_QAMQPQueue::cleanupOnDeletion()
{
    // create, declare, and close the wrong way
    QAmqpQueue *queue = client->createQueue("test-deletion");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->close();
    queue->deleteLater();
    QVERIFY(waitForSignal(queue, SIGNAL(destroyed())));

    // now create, declare, and close the right way
    queue = client->createQueue("test-deletion");
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->close();
    QVERIFY(waitForSignal(queue, SIGNAL(closed())));
}

QTEST_MAIN(tst_QAMQPQueue)
#include "tst_qamqpqueue.moc"
