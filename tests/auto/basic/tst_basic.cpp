#include <QtTest/QtTest>

#include "amqp_client.h"
#include "signalspy.h"

class tst_Basic : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void connect();
    void connectDisconnect();
    void reconnect();

};

void tst_Basic::connect()
{
    QAMQP::Client client;
    SignalSpy spy(&client, SIGNAL(connected()));
    client.connectToHost();
    QVERIFY(spy.wait());
}

void tst_Basic::connectDisconnect()
{
    QAMQP::Client client;
    SignalSpy connectSpy(&client, SIGNAL(connected()));
    client.connectToHost();
    QVERIFY(connectSpy.wait());

    SignalSpy disconnectSpy(&client, SIGNAL(disconnected()));
    client.disconnectFromHost();
    QVERIFY(disconnectSpy.wait());
}

void tst_Basic::reconnect()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_Basic)
#include "tst_basic.moc"
