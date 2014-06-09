#ifndef amqp_client_h__
#define amqp_client_h__

#include <QObject>
#include <QUrl>
#include <QHostAddress>

#ifndef QT_NO_SSL
#include <QSslConfiguration>
#include <QSslError>
#endif

#include "amqp_global.h"

namespace QAMQP
{

class Exchange;
class Queue;
class Authenticator;
class ClientPrivate;
class QAMQP_EXPORT Client : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 port READ port WRITE setPort)
    Q_PROPERTY(QString host READ host WRITE setHost)
    Q_PROPERTY(QString virtualHost READ virtualHost WRITE setVirtualHost)
    Q_PROPERTY(QString user READ username WRITE setUsername)
    Q_PROPERTY(QString password READ password WRITE setPassword)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect)
    Q_PROPERTY(qint16 channelMax READ channelMax WRITE setChannelMax)
    Q_PROPERTY(qint32 frameMax READ frameMax WRITE setFrameMax)
    Q_PROPERTY(qint16 heartbeatDelay READ heartbeatDelay() WRITE setHeartbeatDelay)

public:
    Client(QObject *parent = 0);
    Client(const QUrl &connectionString, QObject *parent = 0);
    ~Client();

    // properties
    quint16 port() const;
    void setPort(quint16 port);

    QString host() const;
    void setHost(const QString &host);

    QString virtualHost() const;
    void setVirtualHost(const QString &virtualHost);

    QString username() const;
    void setUsername(const QString &username);

    QString password() const;
    void setPassword(const QString &password);

    void setAuth(Authenticator *auth);
    Authenticator *auth() const;

    bool autoReconnect() const;
    void setAutoReconnect(bool value);

    bool isConnected() const;

    qint16 channelMax() const;
    void setChannelMax(qint16 channelMax);

    qint32 frameMax() const;
    void setFrameMax(qint32 frameMax);

    qint16 heartbeatDelay() const;
    void setHeartbeatDelay(qint16 delay);

    void addCustomProperty(const QString &name, const QString &value);
    QString customProperty(const QString &name) const;

    enum ConnectionError {
        NoError = 0,
        ConnectionForcedError = 320,
        InvalidPathError = 402,
        FrameError = 501,
        SyntaxError = 502,
        CommandInvalidError = 503,
        ChannelError = 504,
        UnexpectedFrameError = 505,
        ResourceError = 506,
        NotAllowedError = 530,
        NotImplementedError = 540,
        InternalError = 541
    };
    ConnectionError error() const;
    QString errorString() const;

    // channels
    Exchange *createExchange(int channelNumber = -1);
    Exchange *createExchange(const QString &name, int channelNumber = -1);

    Queue *createQueue(int channelNumber = -1);
    Queue *createQueue(const QString &name, int channelNumber = -1);

    // methods
    void connectToHost(const QString &connectionString = QString());
    void connectToHost(const QHostAddress &address, quint16 port = AMQP_PORT);
    void disconnectFromHost();

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(ConnectionError error);

protected:
    Client(ClientPrivate *dd, QObject *parent = 0);

    Q_DISABLE_COPY(Client)
    Q_DECLARE_PRIVATE(Client)
    QScopedPointer<ClientPrivate> d_ptr;

private:
    Q_PRIVATE_SLOT(d_func(), void _q_socketConnected())
    Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_socketError(QAbstractSocket::SocketError error))
    Q_PRIVATE_SLOT(d_func(), void _q_heartbeat())
    Q_PRIVATE_SLOT(d_func(), void _q_connect())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnect())

    friend class ChannelPrivate;

};

#ifndef QT_NO_SSL
class SslClientPrivate;
class SslClient : public Client
{
    Q_OBJECT
public:
    SslClient(QObject *parent = 0);
    SslClient(const QUrl &connectionString, QObject *parent = 0);
    ~SslClient();

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);

private:
    Q_DISABLE_COPY(SslClient)
    Q_DECLARE_PRIVATE(SslClient)

    Q_PRIVATE_SLOT(d_func(), void _q_sslErrors(const QList<QSslError> &errors))

};
#endif

} // namespace QAMQP

#endif // amqp_client_h__
