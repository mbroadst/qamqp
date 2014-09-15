#ifndef QAMQPCLIENT_H
#define QAMQPCLIENT_H

#include <QObject>
#include <QUrl>
#include <QHostAddress>

#ifndef QT_NO_SSL
#include <QSslConfiguration>
#include <QSslError>
#endif

#include "qamqpglobal.h"

class QAmqpExchange;
class QAmqpQueue;
class QAmqpAuthenticator;
class QAmqpClientPrivate;
class QAMQP_EXPORT QAmqpClient : public QObject
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
    explicit QAmqpClient(QObject *parent = 0);
    ~QAmqpClient();

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

    void setAuth(QAmqpAuthenticator *auth);
    QAmqpAuthenticator *auth() const;

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

    QAMQP::Error error() const;
    QString errorString() const;

    // channels
    QAmqpExchange *createExchange(int channelNumber = -1);
    QAmqpExchange *createExchange(const QString &name, int channelNumber = -1);

    QAmqpQueue *createQueue(int channelNumber = -1);
    QAmqpQueue *createQueue(const QString &name, int channelNumber = -1);

    // methods
    void connectToHost(const QString &uri = QString());
    void connectToHost(const QHostAddress &address, quint16 port = AMQP_PORT);
    void disconnectFromHost();

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(QAMQP::Error error);

protected:
    QAmqpClient(QAmqpClientPrivate *dd, QObject *parent = 0);

    Q_DISABLE_COPY(QAmqpClient)
    Q_DECLARE_PRIVATE(QAmqpClient)
    QScopedPointer<QAmqpClientPrivate> d_ptr;

private:
    Q_PRIVATE_SLOT(d_func(), void _q_socketConnected())
    Q_PRIVATE_SLOT(d_func(), void _q_socketDisconnected())
    Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_socketError(QAbstractSocket::SocketError error))
    Q_PRIVATE_SLOT(d_func(), void _q_heartbeat())
    Q_PRIVATE_SLOT(d_func(), void _q_connect())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnect())

    friend class QAmqpChannelPrivate;

};

#ifndef QT_NO_SSL
class QAmqpSslClientPrivate;
class QAmqpSslClient : public QAmqpClient
{
    Q_OBJECT
public:
    QAmqpSslClient(QObject *parent = 0);
    QAmqpSslClient(const QUrl &connectionString, QObject *parent = 0);
    ~QAmqpSslClient();

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);

private:
    Q_DISABLE_COPY(QAmqpSslClient)
    Q_DECLARE_PRIVATE(QAmqpSslClient)

    Q_PRIVATE_SLOT(d_func(), void _q_sslErrors(const QList<QSslError> &errors))

};
#endif

#endif // QAMQPCLIENT_H
