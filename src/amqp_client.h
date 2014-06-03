#ifndef amqp_client_h__
#define amqp_client_h__

#include <QObject>
#include <QUrl>
#include <QHostAddress>

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
    Q_PROPERTY(QString user READ user WRITE setUser)
    Q_PROPERTY(QString password READ password WRITE setPassword)
    Q_PROPERTY(bool ssl READ isSsl WRITE setSsl)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect)
    Q_PROPERTY(bool connected READ isConnected)

public:
    Client(QObject *parent = 0);
    Client(const QUrl &connectionString, QObject *parent = 0);
    ~Client();

    void addCustomProperty(const QString &name, const QString &value);
    QString customProperty(const QString &name) const;

    Exchange *createExchange(int channelNumber = -1);
    Exchange *createExchange(const QString &name, int channelNumber = -1);

    Queue *createQueue(int channelNumber = -1);
    Queue *createQueue(const QString &name, int channelNumber = -1);

    quint16 port() const;
    void setPort(quint16 port);

    QString host() const;
    void setHost(const QString &host);

    QString virtualHost() const;
    void setVirtualHost(const QString &virtualHost);

    QString user() const;
    void setUser(const QString &user);

    QString password() const;
    void setPassword(const QString &password);

    void setAuth(Authenticator *auth);
    Authenticator *auth() const;

    bool isSsl() const;
    void setSsl(bool value);

    bool autoReconnect() const;
    void setAutoReconnect(bool value);

    bool isConnected() const;

    void connectToHost(const QString &connectionString = QString());
    void connectToHost(const QHostAddress &address, quint16 port);
    void disconnectFromHost();

Q_SIGNALS:
    void connected();
    void disconnected();

private:
    Q_DISABLE_COPY(Client)
    Q_DECLARE_PRIVATE(Client)
    QScopedPointer<ClientPrivate> d_ptr;

    friend class ChannelPrivate;

};

} // namespace QAMQP

#endif // amqp_client_h__
