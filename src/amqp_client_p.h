#ifndef amqp_client_p_h__
#define amqp_client_p_h__

#include <QHash>
#include <QSharedPointer>
#include <QPointer>
#include <QAbstractSocket>

#ifndef QT_NO_SSL
#   include <QSslConfiguration>
#   include <QSslError>
#endif

#include "amqp_table.h"
#include "amqp_frame.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

class QTimer;
class QTcpSocket;
namespace QAMQP
{

class Client;
class Queue;
class Exchange;
class Network;
class Connection;
class Authenticator;
class ClientPrivate : public Frame::MethodHandler
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miStart, 10),
        METHOD_ID_ENUM(miSecure, 20),
        METHOD_ID_ENUM(miTune, 30),
        METHOD_ID_ENUM(miOpen, 40),
        METHOD_ID_ENUM(miClose, 50)
    };

    ClientPrivate(Client *q);
    virtual ~ClientPrivate();

    virtual void init();
    virtual void initSocket();
    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void parseConnectionString(const QString &uri);
    void sendFrame(const Frame::Base &frame);

    // private slots
    void _q_socketConnected();
    void _q_socketDisconnected();
    void _q_readyRead();
    void _q_socketError(QAbstractSocket::SocketError error);
    void _q_heartbeat();
    virtual void _q_connect();
    void _q_disconnect();

    virtual bool _q_method(const Frame::Method &frame);

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

    quint16 port;
    QString host;
    QString virtualHost;

    QSharedPointer<Authenticator> authenticator;

    // Network
    QByteArray buffer;
    bool autoReconnect;
    int timeout;
    bool connecting;
    QTcpSocket *socket;

    QHash<quint16, QList<Frame::MethodHandler*> > methodHandlersByChannel;
    QHash<quint16, QList<Frame::ContentHandler*> > contentHandlerByChannel;
    QHash<quint16, QList<Frame::ContentBodyHandler*> > bodyHandlersByChannel;

    // Connection
    bool closed;
    bool connected;
    QPointer<QTimer> heartbeatTimer;
    Table customProperties;
    qint16 channelMax;
    qint16 heartbeatDelay;
    qint32 frameMax;

    Error error;
    QString errorString;

    Client * const q_ptr;
    Q_DECLARE_PUBLIC(Client)

};

#ifndef QT_NO_SSL
class SslClient;
class SslClientPrivate : public ClientPrivate
{
public:
    SslClientPrivate(SslClient *q);

    virtual void initSocket();
    virtual void _q_connect();

    // private slots
    void _q_sslErrors(const QList<QSslError> &errors);

    QSslConfiguration sslConfiguration;

};
#endif

} // namespace QAMQP

#endif // amqp_client_p_h__
