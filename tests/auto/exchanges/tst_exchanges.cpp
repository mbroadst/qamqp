#include <QtTest/QtTest>

class tst_Exchanges : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dummy();

};

void tst_Exchanges::dummy()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_Exchanges)
#include "tst_exchanges.moc"
