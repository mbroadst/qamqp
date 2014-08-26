#include <QtTest/QtTest>
#include "qamqptestcase.h"

#include <QProcess>
#include "qamqpclient.h"
#include "qamqpclient_p.h"
#include "qamqpauthenticator.h"

using namespace QAMQP;
class tst_QAMQPClient : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void connect();
    void connectProperties();
    void connectHostAddress();
    void connectDisconnect();
    void invalidAuthenticationMechanism();
    void tune();

    void validateUri_data();
    void validateUri();

private:
    void autoReconnect();

};

void tst_QAMQPClient::connect()
{
    Client client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));

    QCOMPARE(client.host(), QLatin1String(AMQP_HOST));
    QCOMPARE(client.port(), quint16(AMQP_PORT));
    QCOMPARE(client.virtualHost(), QLatin1String(AMQP_VHOST));
    QCOMPARE(client.username(), QLatin1String(AMQP_LOGIN));
    QCOMPARE(client.password(), QLatin1String(AMQP_PSWD));
    QCOMPARE(client.autoReconnect(), false);

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::connectProperties()
{
    Client client;
    client.setHost("localhost");
    client.setPort(5672);
    client.setVirtualHost("/");
    client.setUsername("guest");
    client.setPassword("guest");
    client.setAutoReconnect(false);
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::connectHostAddress()
{
    Client client;
    client.connectToHost(QHostAddress::LocalHost, 5672);
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
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

void tst_QAMQPClient::validateUri_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("expectedUsername");
    QTest::addColumn<QString>("expectedPassword");
    QTest::addColumn<QString>("expectedHost");
    QTest::addColumn<quint16>("expectedPort");
    QTest::addColumn<QString>("expectedVirtualHost");

    QTest::newRow("standard") << "amqp://user:pass@host:10000/vhost"
        << "user" << "pass" << "host" << quint16(10000) << "vhost";
#if QT_VERSION >= 0x040806
    QTest::newRow("urlencoded") << "amqp://user%61:%61pass@ho%61st:10000/v%2fhost"
        << "usera" << "apass" << "hoast" << quint16(10000) << "v/host";
#endif
    QTest::newRow("empty") << "amqp://" << "" << "" << "" << quint16(AMQP_PORT) << "";
    QTest::newRow("empty2") << "amqp://:@/" << "" << "" << "" << quint16(AMQP_PORT) << "";
    QTest::newRow("onlyuser") << "amqp://user@" << "user" << "" << "" << quint16(AMQP_PORT) << "";
    QTest::newRow("userpass") << "amqp://user:pass@" << "user" << "pass" << "" << quint16(AMQP_PORT) << "";
    QTest::newRow("onlyhost") << "amqp://host" << "" << "" << "host" << quint16(AMQP_PORT) << "";
    QTest::newRow("onlyport") << "amqp://:10000" << "" << "" << "" << quint16(10000) << "";
    QTest::newRow("onlyvhost") << "amqp:///vhost" << "" << "" << "" << quint16(AMQP_PORT) << "vhost";
    QTest::newRow("urlencodedvhost") << "amqp://host/%2f"
        << "" << "" << "host" << quint16(AMQP_PORT) << "/";
    QTest::newRow("ipv6") << "amqp://[::1]" << "" << "" << "::1" << quint16(AMQP_PORT) << "";
}

void tst_QAMQPClient::validateUri()
{
    QFETCH(QString, uri);
    QFETCH(QString, expectedUsername);
    QFETCH(QString, expectedPassword);
    QFETCH(QString, expectedHost);
    QFETCH(quint16, expectedPort);
    QFETCH(QString, expectedVirtualHost);

    ClientPrivate clientPrivate(0);
    // fake init
    clientPrivate.authenticator = QSharedPointer<Authenticator>(
        new AMQPlainAuthenticator(QString::fromLatin1(AMQP_LOGIN), QString::fromLatin1(AMQP_PSWD)));

    // test parsing
    clientPrivate.parseConnectionString(uri);
    AMQPlainAuthenticator *auth =
        static_cast<AMQPlainAuthenticator*>(clientPrivate.authenticator.data());

    QCOMPARE(auth->login(), expectedUsername);
    QCOMPARE(auth->password(), expectedPassword);
    QCOMPARE(clientPrivate.host, expectedHost);
    QCOMPARE(clientPrivate.port, expectedPort);
    QCOMPARE(clientPrivate.virtualHost, expectedVirtualHost);
}

QTEST_MAIN(tst_QAMQPClient)
#include "tst_qamqpclient.moc"
