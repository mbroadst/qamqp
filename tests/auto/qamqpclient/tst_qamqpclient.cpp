#include <QtTest/QtTest>
#include "amqp_testcase.h"

#include "amqp_client.h"

using namespace QAMQP;
class tst_QAMQPClient : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void connect();
    void connectDisconnect();
    void reconnect();

};

void tst_QAMQPClient::connect()
{
    Client client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
}

void tst_QAMQPClient::connectDisconnect()
{
    Client client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::reconnect()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_QAMQPClient)
#include "tst_qamqpclient.moc"
