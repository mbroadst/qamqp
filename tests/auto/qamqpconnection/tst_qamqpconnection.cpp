#include <QtTest/QtTest>
#include "signalspy.h"

class tst_QAMQPConnection : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dummy();

private:
    bool waitForSignal(QObject *obj, const char *signal, int delay = 1);

};

bool tst_QAMQPConnection::waitForSignal(QObject *obj, const char *signal, int delay)
{
    QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QPointer<QObject> safe = obj;

    QTestEventLoop::instance().enterLoop(delay);
    if (!safe.isNull())
    QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    return !QTestEventLoop::instance().timeout();
}

void tst_QAMQPConnection::dummy()
{
    QVERIFY(true);
}

QTEST_MAIN(tst_QAMQPConnection)
#include "tst_qamqpconnection.moc"
