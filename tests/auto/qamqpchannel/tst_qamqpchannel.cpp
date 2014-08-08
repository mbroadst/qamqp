#include <QtTest/QtTest>

#include "signalspy.h"
#include "amqp_testcase.h"

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"
using namespace QAMQP;

class tst_QAMQPChannel : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void close();
    void resume();

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

QTEST_MAIN(tst_QAMQPChannel)
#include "tst_qamqpchannel.moc"
