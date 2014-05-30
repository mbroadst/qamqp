#ifndef amqp_connection_p_h__
#define amqp_connection_p_h__

#include <QPointer>

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

class QTimer;
namespace QAMQP
{

class Client;
class ClientPrivate;
class Connection;
class ConnectionPrivate
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miStart, 10),
        METHOD_ID_ENUM(miSecure, 20),
        METHOD_ID_ENUM(miTune, 30),
        METHOD_ID_ENUM(miOpen, 40),
        METHOD_ID_ENUM(miClose, 50)
    };

    ConnectionPrivate(Connection *q);
    ~ConnectionPrivate();

    void init(Client *parent);
    void startOk();
    void secureOk();
    void tuneOk();
    void open();
    void close(int code, const QString &text, int classId = 0, int methodId = 0);
    void closeOk();

    void start(const Frame::Method &frame);
    void secure(const Frame::Method &frame);
    void tune(const Frame::Method &frame);
    void openOk(const Frame::Method &frame);
    void close(const Frame::Method &frame);
    void closeOk(const Frame::Method &frame);

    bool _q_method(const Frame::Method &frame);
    void _q_heartbeat();

    void setQOS(qint32 prefetchSize, quint16 prefetchCount, int channel, bool global);

    QPointer<Client> client_;
    bool closed_;
    bool connected;
    QPointer<QTimer> heartbeatTimer_;

    Frame::TableField customProperty;

    Q_DECLARE_PUBLIC(Connection)
    Connection * const q_ptr;
};

} // namespace QAMQP

#endif // amqp_connection_p_h__
