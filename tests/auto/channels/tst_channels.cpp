#include <QtTest/QtTest>

class tst_Channels : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dummy();

};

void tst_Channels::dummy()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_Channels)
#include "tst_channels.moc"
