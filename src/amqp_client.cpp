#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_global.h"
#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_connection_p.h"
#include "amqp_authenticator.h"

#include <QTextStream>

using namespace QAMQP;

ClientPrivate::ClientPrivate(Client *q)
    : port(AMQPPORT),
      host(QString::fromLatin1(AMQPHOST)),
      virtualHost(QString::fromLatin1(AMQPVHOST)),
      q_ptr(q)
{
}

ClientPrivate::~ClientPrivate()
{
}

void ClientPrivate::init(const QUrl &connectionString)
{
    Q_Q(Client);
    if (!network_) {
        network_ = new Network(q);
        QObject::connect(network_.data(), SIGNAL(connected()), q, SIGNAL(connected()));
        QObject::connect(network_.data(), SIGNAL(disconnected()), q, SIGNAL(disconnected()));
    }

    if (!connection_)
        connection_ = new Connection(q);
    network_->setMethodHandlerConnection(connection_);

    auth_ = QSharedPointer<Authenticator>(
                new AMQPlainAuthenticator(QString::fromLatin1(AMQPLOGIN), QString::fromLatin1(AMQPPSWD)));

    QObject::connect(connection_, SIGNAL(connected()), q, SIGNAL(connected()));
    QObject::connect(connection_, SIGNAL(disconnected()), q, SIGNAL(disconnected()));

    if (connectionString.isValid()) {
        parseConnectionString(connectionString);
        connect();
    }
}

void ClientPrivate::connect()
{
    sockConnect();
}

void ClientPrivate::parseConnectionString(const QUrl &connectionString)
{
    Q_Q(Client);
    if (connectionString.scheme() != AMQPSCHEME &&
        connectionString.scheme() != AMQPSSCHEME) {
        qDebug() << Q_FUNC_INFO << "invalid scheme: " << connectionString.scheme();
        return;
    }

    q->setSsl(connectionString.scheme() == AMQPSSCHEME);
    q->setPassword(connectionString.password());
    q->setUser(connectionString.userName());
    q->setPort(connectionString.port(AMQPPORT));
    q->setHost(connectionString.host());
    q->setVirtualHost(connectionString.path());
}

void ClientPrivate::sockConnect()
{
    if (network_->state() != QAbstractSocket::UnconnectedState)
        disconnect();
    network_->connectTo(host, port);
}

void ClientPrivate::disconnect()
{
    if (network_->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << Q_FUNC_INFO << "already disconnected";
        return;
    }

    network_->disconnect();
    connection_->d_func()->connected = false;
}

//////////////////////////////////////////////////////////////////////////

Client::Client(QObject *parent)
    : QObject(parent),
      d_ptr(new ClientPrivate(this))
{
    Q_D(Client);
    d->init();
}

Client::Client(const QUrl &connectionString, QObject *parent)
    : QObject(parent),
      d_ptr(new ClientPrivate(this))
{
    Q_D(Client);
    d->init(connectionString);
}

Client::~Client()
{
}

quint16 Client::port() const
{
    Q_D(const Client);
    return d->port;
}

void Client::setPort(quint16 port)
{
    Q_D(Client);
    d->port = port;
}

QString Client::host() const
{
    Q_D(const Client);
    return d->host;
}

void Client::setHost(const QString &host)
{
    Q_D(Client);
    d->host = host;
}

QString Client::virtualHost() const
{
    Q_D(const Client);
    return d->virtualHost;
}

void Client::setVirtualHost(const QString &virtualHost)
{
    Q_D(Client);
    d->virtualHost = virtualHost;
}

QString Client::user() const
{
    Q_D(const Client);
    const Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const AMQPlainAuthenticator *a = static_cast<const AMQPlainAuthenticator*>(auth);
        return a->login();
    }

    return QString();
}

void Client::setUser(const QString &user)
{
    Q_D(const Client);
    Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator *a = static_cast<AMQPlainAuthenticator*>(auth);
        a->setLogin(user);
    }
}

QString Client::password() const
{
    Q_D(const Client);
    const Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == "AMQPLAIN") {
        const AMQPlainAuthenticator *a = static_cast<const AMQPlainAuthenticator*>(auth);
        return a->password();
    }

    return QString();
}

void Client::setPassword(const QString &password)
{
    Q_D(Client);
    Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator *a = static_cast<AMQPlainAuthenticator*>(auth);
        a->setPassword(password);
    }
}

Exchange *Client::createExchange(int channelNumber)
{
    return createExchange(QString(), channelNumber);
}

Exchange *Client::createExchange(const QString &name, int channelNumber)
{
    Q_D(Client);
    Exchange *exchange = new Exchange(channelNumber, this);
    d->network_->addMethodHandlerForChannel(exchange->channelNumber(), exchange);
    connect(d->connection_, SIGNAL(connected()), exchange, SLOT(_q_open()));
    exchange->d_func()->open();
    connect(this, SIGNAL(disconnected()), exchange, SLOT(_q_disconnected()));
    if (!name.isEmpty())
        exchange->setName(name);
    return exchange;
}

Queue *Client::createQueue(int channelNumber)
{
    return createQueue(QString(), channelNumber);
}

Queue *Client::createQueue(const QString &name, int channelNumber)
{
    Q_D(Client);
    Queue *queue = new Queue(channelNumber, this);
    d->network_->addMethodHandlerForChannel(queue->channelNumber(), queue);
    d->network_->addContentHandlerForChannel(queue->channelNumber(), queue);
    d->network_->addContentBodyHandlerForChannel(queue->channelNumber(), queue);

    connect(d->connection_, SIGNAL(connected()), queue, SLOT(_q_open()));
    queue->d_func()->open();
    connect(this, SIGNAL(disconnected()), queue, SLOT(_q_disconnected()));

    if (!name.isEmpty())
        queue->setName(name);
    return queue;
}

void Client::setAuth(Authenticator *auth)
{
    Q_D(Client);
    d->auth_ = QSharedPointer<Authenticator>(auth);
}

Authenticator *Client::auth() const
{
    Q_D(const Client);
    return d->auth_.data();
}

bool Client::isSsl() const
{
    Q_D(const Client);
    return d->network_->isSsl();
}

void Client::setSsl(bool value)
{
    Q_D(Client);
    d->network_->setSsl(value);
}

bool Client::autoReconnect() const
{
    Q_D(const Client);
    return d->network_->autoReconnect();
}

void Client::setAutoReconnect(bool value)
{
    Q_D(Client);
    d->network_->setAutoReconnect(value);
}

bool Client::isConnected() const
{
    Q_D(const Client);
    return d->connection_->isConnected();
}

void Client::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(Client);
    return d->connection_->addCustomProperty(name, value);
}

QString Client::customProperty(const QString &name) const
{
    Q_D(const Client);
    return d->connection_->customProperty(name);
}

void Client::connectToHost(const QString &connectionString)
{
    Q_D(Client);
    if (connectionString.isEmpty()) {
        d->connect();
        return;
    }

    d->parseConnectionString(QUrl::fromUserInput(connectionString));
    d->connect();
}

void Client::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(Client);
    d->host = address.toString();
    d->port = port;
    d->connect();
}

void Client::disconnectFromHost()
{
    Q_D(Client);
    d->disconnect();
}
