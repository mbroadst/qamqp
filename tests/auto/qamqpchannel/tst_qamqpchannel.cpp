#include <QtTest/QtTest>

#include "signalspy.h"
#include "qamqptestcase.h"

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
using namespace QAMQP;

class tst_QAMQPChannel : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void close();
    void resume();
    void sharedChannel();

private:
    QScopedPointer<Client> client;

};

void tst_QAMQPChannel::init()
{
    client.reset(new Client);
    client->connectToHost();
    QVERIFY(waitForSignal(client.data(), SIGNAL(connected())));
}

void tst_QAMQPChannel::cleanup()
{
    if (client->isConnected()) {
        client->disconnectFromHost();
        QVERIFY(waitForSignal(client.data(), SIGNAL(disconnected())));
    }
}

void tst_QAMQPChannel::close()
{
    // exchange
    Exchange *exchange = client->createExchange("test-close-channel");
    QVERIFY(waitForSignal(exchange, SIGNAL(opened())));
    exchange->declare(Exchange::Direct);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    exchange->close();
    QVERIFY(waitForSignal(exchange, SIGNAL(closed())));
    exchange->reopen();
    QVERIFY(waitForSignal(exchange, SIGNAL(opened())));
    exchange->remove(Exchange::roForce);
    QVERIFY(waitForSignal(exchange, SIGNAL(removed())));

    // queue
    Queue *queue = client->createQueue("test-close-channel");
    QVERIFY(waitForSignal(queue, SIGNAL(opened())));
    declareQueueAndVerifyConsuming(queue);
    queue->close();
    QVERIFY(waitForSignal(queue, SIGNAL(closed())));
}

void tst_QAMQPChannel::resume()
{
    Queue *queue = client->createQueue("test-resume");
    QVERIFY(waitForSignal(queue, SIGNAL(opened())));
    declareQueueAndVerifyConsuming(queue);

    queue->resume();
    QVERIFY(waitForSignal(queue, SIGNAL(resumed())));
}

void tst_QAMQPChannel::sharedChannel()
{
    QString routingKey = "test-shared-channel";
    Queue *queue = client->createQueue(routingKey);
    declareQueueAndVerifyConsuming(queue);

    Exchange *defaultExchange = client->createExchange("", queue->channelNumber());
    defaultExchange->publish("first message", routingKey);
    QVERIFY(waitForSignal(queue, SIGNAL(messageReceived())));
    Message message = queue->dequeue();
    verifyStandardMessageHeaders(message, routingKey);
    QCOMPARE(message.payload(), QByteArray("first message"));
}

QTEST_MAIN(tst_QAMQPChannel)
#include "tst_qamqpchannel.moc"
