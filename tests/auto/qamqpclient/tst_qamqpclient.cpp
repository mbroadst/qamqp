#include <QtTest/QtTest>
#include <QProcess>
#include <QSslKey>

#include "qamqptestcase.h"
#include "qamqpauthenticator.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
#include "qamqpclient_p.h"
#include "qamqpclient.h"

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
    void socketError();
    void validateUri_data();
    void validateUri();
    void issue38();
    void issue38_take2();

public Q_SLOTS:     // temporarily disabled
    void autoReconnect();
    void autoReconnectTimeout();
    void sslConnect();

private:
    QSslConfiguration createSslConfiguration();
    void issue38_helper(QAmqpClient *client);

};

QSslConfiguration tst_QAMQPClient::createSslConfiguration()
{
    QList<QSslCertificate> caCerts =
        QSslCertificate::fromPath(QLatin1String(":/certs/ca-cert.pem"));
    QList<QSslCertificate> localCerts =
        QSslCertificate::fromPath(QLatin1String(":/certs/client-cert.pem"));
    QFile keyFile( QLatin1String(":/certs/client-key.pem"));
    keyFile.open(QIODevice::ReadOnly);
    QSslKey key(&keyFile, QSsl::Rsa, QSsl::Pem,  QSsl::PrivateKey);
    keyFile.close();

    QSslConfiguration sslConfiguration;
    sslConfiguration.setCaCertificates(caCerts);
    sslConfiguration.setLocalCertificate(localCerts.first());
    sslConfiguration.setPrivateKey(key);
    sslConfiguration.setProtocol(QSsl::SecureProtocols);
    return sslConfiguration;
}

void tst_QAMQPClient::connect()
{
    QAmqpClient client;
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

void tst_QAMQPClient::sslConnect()
{
    QAmqpClient client;
    client.setSslConfiguration(createSslConfiguration());
    QObject::connect(&client, SIGNAL(sslErrors(QList<QSslError>)),
                     &client, SLOT(ignoreSslErrors(QList<QSslError>)));

    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
}

void tst_QAMQPClient::connectProperties()
{
    QAmqpClient client;
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
    QAmqpClient client;
    client.connectToHost(QHostAddress::LocalHost, 5672);
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::connectDisconnect()
{
    QAmqpClient client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

class InvalidAuthenticator : public QAmqpAuthenticator
{
public:
    virtual QString type() const { return "CRAZYAUTH"; }
    virtual void write(QDataStream &out) {
        Q_UNUSED(out);
    }
};

void tst_QAMQPClient::invalidAuthenticationMechanism()
{
    QAmqpClient client;
    client.setAuth(new InvalidAuthenticator);
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::autoReconnect()
{
    // TODO: this is a fairly crude way of testing this, research
    //       better alternatives

    QAmqpClient client;
    client.setAutoReconnect(true);
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    QProcess::execute("rabbitmqctl", QStringList() << "stop_app");
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
    QProcess::execute("rabbitmqctl", QStringList() << "start_app");
    QVERIFY(waitForSignal(&client, SIGNAL(connected()), 2));
}

void tst_QAMQPClient::autoReconnectTimeout()
{
    // TODO: this is a fairly crude way of testing this, research
    //       better alternatives

    QAmqpClient client;
    client.setAutoReconnect(true, 3);
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected()), 60));
    qDebug() <<"connected" ;
    QProcess::execute("rabbitmqctl", QStringList() << "stop_app");
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected()), 60));
    qDebug() <<"disconnected" ;
    QProcess::execute("rabbitmqctl", QStringList() << "start_app");
    QVERIFY(waitForSignal(&client, SIGNAL(connected()), 60));
    qDebug() <<"connected" ;

    QVERIFY(waitForSignal(&client, SIGNAL(disconnected()), 60));
    QProcess::execute("rabbitmqctl", QStringList() << "start_app");
    QVERIFY(waitForSignal(&client, SIGNAL(connected()), 60));
}

void tst_QAMQPClient::tune()
{
    QAmqpClient client;
    client.setChannelMax(15);
    client.setFrameMax(5000);
    client.setHeartbeatDelay(600);

    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    QCOMPARE(client.channelMax(), qint16(15));
    QCOMPARE(client.heartbeatDelay(), qint16(600));
    QCOMPARE(client.frameMax(), qint32(5000));

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::socketError()
{
    QAmqpClient client;
    client.connectToHost("amqp://127.0.0.1:56725/");
    QVERIFY(waitForSignal(&client, SIGNAL(socketError(QAbstractSocket::SocketError))));
    QCOMPARE(client.socketError(), QAbstractSocket::ConnectionRefusedError);
}

void tst_QAMQPClient::validateUri_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("expectedUsername");
    QTest::addColumn<QString>("expectedPassword");
    QTest::addColumn<QString>("expectedHost");
    QTest::addColumn<quint16>("expectedPort");
    QTest::addColumn<QString>("expectedVirtualHost");

    QTest::newRow("default vhost") << "amqp://guest:guest@192.168.1.10:5672/"
        << "guest" << "guest" << "192.168.1.10" << quint16(5672) << "/";
    QTest::newRow("standard") << "amqp://user:pass@host:10000/vhost"
        << "user" << "pass" << "host" << quint16(10000) << "vhost";
#if QT_VERSION >= 0x040806
    QTest::newRow("urlencoded") << "amqp://user%61:%61pass@ho%61st:10000/v%2fhost"
        << "usera" << "apass" << "hoast" << quint16(10000) << "v/host";
#endif
    QTest::newRow("empty") << "amqp://" << "" << "" << "" << quint16(AMQP_PORT) << "";
    QTest::newRow("empty2") << "amqp://:@/" << "" << "" << "" << quint16(AMQP_PORT) << "/";
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

    QAmqpClientPrivate clientPrivate(0);
    // fake init
    clientPrivate.authenticator = QSharedPointer<QAmqpAuthenticator>(
        new QAmqpPlainAuthenticator(QString::fromLatin1(AMQP_LOGIN), QString::fromLatin1(AMQP_PSWD)));

    // test parsing
    clientPrivate.parseConnectionString(uri);
    QAmqpPlainAuthenticator *auth =
        static_cast<QAmqpPlainAuthenticator*>(clientPrivate.authenticator.data());

    QCOMPARE(auth->login(), expectedUsername);
    QCOMPARE(auth->password(), expectedPassword);
    QCOMPARE(clientPrivate.host, expectedHost);
    QCOMPARE(clientPrivate.port, expectedPort);
    QCOMPARE(clientPrivate.virtualHost, expectedVirtualHost);
}

void tst_QAMQPClient::issue38_helper(QAmqpClient *client)
{
    // connect
    client->connectToHost();
    QVERIFY(waitForSignal(client, SIGNAL(connected())));

    // create then declare, remove and close queue
    QAmqpQueue *queue = client->createQueue();
    QVERIFY(waitForSignal(queue, SIGNAL(opened())));
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->remove(QAmqpExchange::roForce);
    QVERIFY(waitForSignal(queue, SIGNAL(removed())));
    queue->close();
    QVERIFY(waitForSignal(queue, SIGNAL(closed())));
    queue->deleteLater();

    // disconnect
    client->disconnectFromHost();
    QVERIFY(waitForSignal(client, SIGNAL(disconnected())));
}

void tst_QAMQPClient::issue38()
{
    QAmqpClient client;
    issue38_helper(&client);
    issue38_helper(&client);
}

void tst_QAMQPClient::issue38_take2()
{
    QAmqpClient client;
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    QAmqpExchange *exchange = client.createExchange("myexchange");
    exchange->declare(QAmqpExchange::Topic);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    QAmqpQueue *queue = client.createQueue();
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->bind(exchange, "routingKeyin");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->consume(QAmqpQueue::coNoAck);
    QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));
    exchange->publish("test message", "routingKeyout");

    // delete everything
    queue->unbind(exchange, "routingKeyin");
    QVERIFY(waitForSignal(queue, SIGNAL(unbound())));
    queue->close();
    QVERIFY(waitForSignal(queue, SIGNAL(closed())));
    queue->deleteLater();

    exchange->close();
    QVERIFY(waitForSignal(exchange, SIGNAL(closed())));
    exchange->deleteLater();

    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client,SIGNAL(disconnected())));

    // repeat the connection and create again here
    client.connectToHost();
    QVERIFY(waitForSignal(&client, SIGNAL(connected())));
    exchange = client.createExchange("myexchange");
    exchange->declare(QAmqpExchange::Topic);
    QVERIFY(waitForSignal(exchange, SIGNAL(declared())));
    queue = client.createQueue();
    queue->declare();
    QVERIFY(waitForSignal(queue, SIGNAL(declared())));
    queue->bind(exchange, "routingKeyin");
    QVERIFY(waitForSignal(queue, SIGNAL(bound())));
    queue->consume(QAmqpQueue::coNoAck);
    QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));
    exchange->publish("test message", "routingKeyout");
    client.disconnectFromHost();
    QVERIFY(waitForSignal(&client,SIGNAL(disconnected())));
}


QTEST_MAIN(tst_QAMQPClient)
#include "tst_qamqpclient.moc"
