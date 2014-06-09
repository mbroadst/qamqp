#ifndef amqp_testcase_h__
#define amqp_testcase_h__

#include <QObject>
#include <QTestEventLoop>

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
};

}   // namespace QAMQP

#endif  // amqp_testcase_h__
