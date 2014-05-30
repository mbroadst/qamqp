#ifndef amqp_connection_h__
#define amqp_connection_h__

#include <QObject>
#include "amqp_frame.h"

namespace QAMQP
{

class Client;
class ClientPrivate;
class ChannelPrivate;
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

public Q_SLOTS:
    void startOk();
    void secureOk();
    void tuneOk();
    void open();
    void close(int code, const QString &text, int classId = 0, int methodId = 0);
    void closeOk();

    void setQOS(qint32 prefetchSize, quint16 prefetchCount);

Q_SIGNALS:
    void disconnected();
    void connected();

private:
    explicit Connection(Client *parent = 0);

    Q_DISABLE_COPY(Connection)
    Q_DECLARE_PRIVATE(Connection)
    QScopedPointer<ConnectionPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_heartbeat())
    friend class ClientPrivate;
    friend class ChannelPrivate;

    // should be moved to private
    void openOk();
    void _q_method(const Frame::Method &frame);
};

} // namespace QAMQP

#endif // amqp_connection_h__
