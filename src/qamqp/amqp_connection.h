#ifndef amqp_connection_h__
#define amqp_connection_h__

#include <QObject>
#include "amqp_frame.h"
#include "amqp_global.h"

namespace QAMQP
{

class Client;
class ClientPrivate;
class ChannelPrivate;
class ConnectionPrivate;
class Connection : public QObject, public Frame::MethodHandler
{
    Q_OBJECT
public:
    virtual ~Connection();

    void addCustomProperty(const QString &name, const QString &value);
    QString customProperty(const QString &name) const;

    void startOk();
    void secureOk();
    void tuneOk();
    void open();
    void close(int code, const QString &text, int classId = 0, int methodId = 0);
    void closeOk();

    bool isConnected() const;

    void setQOS(qint32 prefetchSize, quint16 prefetchCount);

Q_SIGNALS:
    void disconnected();
    void connected();

private:
    Q_DISABLE_COPY(Connection)
    Q_DECLARE_PRIVATE(Connection)
    QScopedPointer<ConnectionPrivate> d_ptr;

    Connection(Client * parent = 0);

    void openOk();
    friend class ClientPrivate;
    friend class ChannelPrivate;

    void _q_method(const QAMQP::Frame::Method &frame);
    Q_PRIVATE_SLOT(d_func(), void _q_heartbeat())
};

}

#endif // amqp_connection_h__
