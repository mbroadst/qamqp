#include <QtTest/QtTest>
#include "amqp_testcase.h"

#include <QProcess>
#include "amqp_client.h"

using namespace QAMQP;
class tst_QAMQPClient : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void connect();
    void connectDisconnect();

private:
    void autoReconnect();

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

void tst_QAMQPClient::autoReconnect()
{
    // TODO: this is a fairly crude way of testing this, research
    //       better alternatives

    Client client;
    client.setAutoReconnect(true);
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    QProcess::execute("rabbitmqctl", QStringList() << "stop_app");
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
    QProcess::execute("rabbitmqctl", QStringList() << "start_app");
    QVERIFY(waitForSignal(&client, SIGNAL(connected()), 2));

}

QTEST_MAIN(tst_QAMQPClient)
#include "tst_qamqpclient.moc"
