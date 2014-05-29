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

ClientPrivate::ClientPrivate(Client * q)
    : port(AMQPPORT),
      host(QString::fromLatin1(AMQPHOST)),
      virtualHost(QString::fromLatin1(AMQPVHOST)),
      q_ptr(q)
{
}

ClientPrivate::~ClientPrivate()
{
}

void ClientPrivate::init(QObject *parent)
{
    Q_Q(Client);
    q->setParent(parent);
    if (!network_) {
        network_ = new Network(q);
        QObject::connect(network_.data(), SIGNAL(connected()), q, SIGNAL(connected()));
        QObject::connect(network_.data(), SIGNAL(disconnected()), q, SIGNAL(disconnected()));
    }

    if (!connection_)
        connection_ = new Connection(q);
    network_->setMethodHandlerConnection(connection_);

    setAuth(new AMQPlainAuthenticator(QString::fromLatin1(AMQPLOGIN), QString::fromLatin1(AMQPPSWD)));

    QObject::connect(connection_, SIGNAL(connected()), q, SIGNAL(connected()));
    QObject::connect(connection_, SIGNAL(disconnected()), q, SIGNAL(disconnected()));
}

void ClientPrivate::init(QObject *parent, const QUrl &connectionString)
{
    init(parent);
    parseConnectionString(connectionString);
    connect();
}

void ClientPrivate::setAuth(Authenticator *auth)
{
    auth_ = QSharedPointer<Authenticator>(auth);
}

void ClientPrivate::printConnect() const
{
    QTextStream stream(stdout);
    stream <<  "port  = " << port << endl;
    stream <<  "host  = " << host << endl;
    stream <<  "vhost = " << virtualHost << endl;

    if (auth_ && auth_->type() == QLatin1String("AMQPLAIN")) {
        QSharedPointer<AMQPlainAuthenticator> a = auth_.staticCast<AMQPlainAuthenticator>();
        stream <<  "user  = " << a->login() << endl;
        stream <<  "passw = " << a->password() << endl;
    }
}

void ClientPrivate::connect()
{
    sockConnect();
    login();
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

void ClientPrivate::login()
{
}

Exchange *ClientPrivate::createExchange(int channelNumber, const QString &name)
{
    Q_Q(Client);
    Exchange * exchange_ = new Exchange(channelNumber, q);

    network_->addMethodHandlerForChannel(exchange_->channelNumber(), exchange_);

    QObject::connect(connection_, SIGNAL(connected()), exchange_, SLOT(_q_open()));
    exchange_->d_func()->open();
    QObject::connect(q, SIGNAL(disconnected()), exchange_, SLOT(_q_disconnected()));
    exchange_->setName(name);

    return exchange_;
}

Queue *ClientPrivate::createQueue(int channelNumber, const QString &name )
{
    Q_Q(Client);
    Queue *queue_ = new Queue(channelNumber, q);

    network_->addMethodHandlerForChannel(queue_->channelNumber(), queue_);
    network_->addContentHandlerForChannel(queue_->channelNumber(), queue_);
    network_->addContentBodyHandlerForChannel(queue_->channelNumber(), queue_);

    QObject::connect(connection_, SIGNAL(connected()), queue_, SLOT(_q_open()));
    queue_->d_func()->open();
    QObject::connect(q, SIGNAL(disconnected()), queue_, SLOT(_q_disconnected()));
    queue_->setName(name);

    return queue_;
}

void ClientPrivate::disconnect()
{
//    Q_Q(Client);
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
    d_ptr->init(parent);
}

Client::Client(const QUrl & connectionString, QObject * parent)
    : d_ptr(new ClientPrivate(this))
{
    d_ptr->init(parent, connectionString);
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

void Client::setHost( const QString & host )
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
    const Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const AMQPlainAuthenticator * a = static_cast<const AMQPlainAuthenticator *>(auth);
        return a->login();
    }

    return QString();
}

void Client::setUser(const QString &user)
{
    Q_D(const Client);
    Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator * a = static_cast<AMQPlainAuthenticator *>(auth);
        a->setLogin(user);
    }
}

QString Client::password() const
{
    Q_D(const Client);
    const Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == "AMQPLAIN") {
        const AMQPlainAuthenticator * a = static_cast<const AMQPlainAuthenticator *>(auth);
        return a->password();
    }

    return QString();
}

void Client::setPassword(const QString &password)
{
    Q_D(Client);
    Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator * a = static_cast<AMQPlainAuthenticator *>(auth);
        a->setPassword(password);
    }
}

void Client::printConnect() const
{
#ifdef _DEBUG
    Q_D(const Client);
    d->printConnect();
#endif // _DEBUG
}

void Client::closeChannel()
{
}

Exchange *Client::createExchange(int channelNumber)
{
    Q_D(Client);
    return d->createExchange(channelNumber, QString());
}

Exchange *Client::createExchange( const QString &name, int channelNumber )
{
    Q_D(Client);
    return d->createExchange(channelNumber, name);
}

Queue *Client::createQueue(int channelNumber)
{
    Q_D(Client);
    return d->createQueue(channelNumber, QString());
}

Queue *Client::createQueue( const QString &name, int channelNumber )
{
    Q_D(Client);
    return d->createQueue(channelNumber, name);
}

void Client::setAuth(Authenticator *auth)
{
    Q_D(Client);
    d->setAuth(auth);
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
