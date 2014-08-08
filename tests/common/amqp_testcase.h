#ifndef amqp_testcase_h__
#define amqp_testcase_h__

#include <QObject>
#include <QTestEventLoop>

#include "amqp_queue.h"

namespace QAMQP {

class TestCase : public QObject
{
public:
    TestCase() {}
    virtual ~TestCase() {}

protected:
    bool waitForSignal(QObject *obj, const char *signal, int delay = 5)
    {
        QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
        QPointer<QObject> safe = obj;

        QTestEventLoop::instance().enterLoop(delay);
        if (!safe.isNull())
            QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
        return !QTestEventLoop::instance().timeout();
    }

    void declareQueueAndVerifyConsuming(Queue *queue)
    {
        queue->declare();
        QVERIFY(waitForSignal(queue, SIGNAL(declared())));
        QVERIFY(queue->consume());
        QSignalSpy spy(queue, SIGNAL(consuming(QString)));
        QVERIFY(waitForSignal(queue, SIGNAL(consuming(QString))));
        QVERIFY(queue->isConsuming());
        QVERIFY(!spy.isEmpty());
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), queue->consumerTag());
    }
};

}   // namespace QAMQP

#endif  // amqp_testcase_h__
