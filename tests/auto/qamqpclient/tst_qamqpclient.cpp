#include <QtTest/QtTest>
#include "amqp_testcase.h"

#include <QProcess>
#include "amqp_client.h"
#include "amqp_authenticator.h"

using namespace QAMQP;
class tst_QAMQPClient : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void connect();
    void connectDisconnect();
    void invalidAuthenticationMechanism();

    void tune();

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

class InvalidAuthenticator : public Authenticator
{
public:
    virtual QString type() const { return "CRAZYAUTH"; }
    virtual void write(QDataStream &out) {
        Q_UNUSED(out);
    }
};

void tst_QAMQPClient::invalidAuthenticationMechanism()
{
    Client client;
    client.setAuth(new InvalidAuthenticator);
    client.connectToHost();
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

void tst_QAMQPClient::tune()
{
    Client client;
    client.setChannelMax(15);
    client.setFrameMax(5000);
    client.setHeartbeatDelay(600);

    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    QCOMPARE((int)client.channelMax(), 15);
    QCOMPARE((int)client.heartbeatDelay(), 600);
    QCOMPARE((int)client.frameMax(), 5000);

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

QTEST_MAIN(tst_QAMQPClient)
#include "tst_qamqpclient.moc"
