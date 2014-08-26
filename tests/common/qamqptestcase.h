#ifndef QAMQPTESTCASE_H
#define QAMQPTESTCASE_H

#include <QObject>
#include <QTestEventLoop>

#include "qamqpqueue.h"

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

    void verifyStandardMessageHeaders(const Message &message, const QString &routingKey,
                                      const QString &exchangeName = QLatin1String(""),
                                      bool redelivered = false)
    {
        QCOMPARE(message.routingKey(), routingKey);
        QCOMPARE(message.exchangeName(), exchangeName);
        QCOMPARE(message.isRedelivered(), redelivered);
    }

};

}   // namespace QAMQP

#endif  // QAMQPTESTCASE_H
