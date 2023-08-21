#ifndef SIGNALSPY_H
#define SIGNALSPY_H

namespace QAMQP {
namespace Test {

bool waitForSignal(QObject *obj, const char *signal, int delay)
{
    QObject::connect(obj, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QPointer<QObject> safe = obj;

    QTestEventLoop::instance().enterLoop(delay);
    if (!safe.isNull())
    QObject::disconnect(safe, signal, &QTestEventLoop::instance(), SLOT(exitLoop()));
    return !QTestEventLoop::instance().timeout();
}

} // namespace Test
} // namespace QAMQP
#endif
