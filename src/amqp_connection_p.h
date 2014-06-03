#ifndef amqp_connection_p_h__
#define amqp_connection_p_h__

#include <QObject>
#include "amqp_frame.h"

namespace QAMQP
{

class Client;
class Network;
class ClientPrivate;
class ConnectionPrivate;
class QAMQP_EXPORT Connection : public QObject, public Frame::MethodHandler
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected CONSTANT)

public:
    virtual ~Connection();

    void addCustomProperty(const QString &name, const QString &value);
    QString customProperty(const QString &name) const;

    bool isConnected() const;
    void setQOS(qint32 prefetchSize, quint16 prefetchCount);

    // method handlers, FROM server
    void start(const Frame::Method &frame);
    void secure(const Frame::Method &frame);
    void tune(const Frame::Method &frame);
    void openOk(const Frame::Method &frame);
    void closeOk(const Frame::Method &frame);

    // method handlers, TO server
    void startOk();
    void secureOk();
    void tuneOk();
    void open();

    // method handlers, BOTH ways
    void close(int code, const QString &text, int classId = 0, int methodId = 0);
    void close(const Frame::Method &frame);
    void closeOk();

Q_SIGNALS:
    void disconnected();
    void connected();

private:
    explicit Connection(Network *network, Client *parent);

    Q_DISABLE_COPY(Connection)
    Q_DECLARE_PRIVATE(Connection)
    QScopedPointer<ConnectionPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_heartbeat())
    friend class ClientPrivate;

    // should be moved to private
    void openOk();
    void _q_method(const Frame::Method &frame);
};

} // namespace QAMQP

#endif // amqp_connection_h__
