#include <QtTest/QtTest>

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
    QVERIFY(true);
}

void tst_Basic::connectDisconnect()
{
    QVERIFY(true);
}

void tst_Basic::reconnect()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_Basic)
#include "tst_basic.moc"
