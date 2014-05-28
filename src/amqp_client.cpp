#include "amqp_client.h"
#include "amqp_client_p.h"

#include <QTextStream>
#include <QCoreApplication>
#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_connection_p.h"
#include "amqp_authenticator.h"

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
    Q_Q(QAMQP::Client);
    q->setParent(parent);
    if (!network_)
        network_ = new QAMQP::Network(q);

    if (!connection_)
        connection_ = new QAMQP::Connection(q);
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
    Q_Q(QAMQP::Client);
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
    Q_Q(QAMQP::Client);
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
    Q_Q(QAMQP::Client);
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
    Q_Q(QAMQP::Client);
    if (network_->state() != QAbstractSocket::UnconnectedState) {
        network_->disconnect();
        connection_->d_func()->connected = false;
        Q_EMIT q->disconnected();
    }
}

//////////////////////////////////////////////////////////////////////////

QAMQP::Client::Client(QObject *parent)
    : QObject(parent),
      d_ptr(new ClientPrivate(this))
{
    d_ptr->init(parent);
}

QAMQP::Client::Client(const QUrl & connectionString, QObject * parent)
    : d_ptr(new ClientPrivate(this))
{
    d_ptr->init(parent, connectionString);
}

QAMQP::Client::~Client()
{
}

quint16 QAMQP::Client::port() const
{
    Q_D(const QAMQP::Client);
    return d->port;
}

void QAMQP::Client::setPort(quint16 port)
{
    Q_D(QAMQP::Client);
    d->port = port;
}

QString QAMQP::Client::host() const
{
    Q_D(const QAMQP::Client);
    return d->host;
}

void QAMQP::Client::setHost( const QString & host )
{
    Q_D(QAMQP::Client);
    d->host = host;
}

QString QAMQP::Client::virtualHost() const
{
    Q_D(const QAMQP::Client);
    return d->virtualHost;
}

void QAMQP::Client::setVirtualHost(const QString &virtualHost)
{
    Q_D(QAMQP::Client);
    d->virtualHost = virtualHost;
}

QString QAMQP::Client::user() const
{
    Q_D(const QAMQP::Client);
    const Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const AMQPlainAuthenticator * a = static_cast<const AMQPlainAuthenticator *>(auth);
        return a->login();
    }

    return QString();
}

void QAMQP::Client::setUser(const QString &user)
{
    Q_D(const QAMQP::Client);
    Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator * a = static_cast<AMQPlainAuthenticator *>(auth);
        a->setLogin(user);
    }
}

QString QAMQP::Client::password() const
{
    Q_D(const QAMQP::Client);
    const Authenticator * auth = d->auth_.data();
    if (auth && auth->type() == "AMQPLAIN") {
        const AMQPlainAuthenticator * a = static_cast<const AMQPlainAuthenticator *>(auth);
        return a->password();
    }

    return QString();
}

void QAMQP::Client::setPassword(const QString &password)
{
    Q_D(QAMQP::Client);
    Authenticator *auth = d->auth_.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator * a = static_cast<AMQPlainAuthenticator *>(auth);
        a->setPassword(password);
    }
}

void QAMQP::Client::printConnect() const
{
#ifdef _DEBUG
    Q_D(const QAMQP::Client);
    d->printConnect();
#endif // _DEBUG
}

void QAMQP::Client::closeChannel()
{
}

Exchange *QAMQP::Client::createExchange(int channelNumber)
{
    Q_D(QAMQP::Client);
    return d->createExchange(channelNumber, QString());
}

Exchange *QAMQP::Client::createExchange( const QString &name, int channelNumber )
{
    Q_D(QAMQP::Client);
    return d->createExchange(channelNumber, name);
}

Queue *QAMQP::Client::createQueue(int channelNumber)
{
    Q_D(QAMQP::Client);
    return d->createQueue(channelNumber, QString());
}

Queue *QAMQP::Client::createQueue( const QString &name, int channelNumber )
{
    Q_D(QAMQP::Client);
    return d->createQueue(channelNumber, name);
}

void QAMQP::Client::open()
{
    Q_D(QAMQP::Client);
    return d->connect();
}

void QAMQP::Client::open(const QUrl &connectionString)
{
    Q_D(QAMQP::Client);
    d->parseConnectionString(connectionString);
    open();
}

void QAMQP::Client::close()
{
    Q_D(QAMQP::Client);
    return d->disconnect();
}

void QAMQP::Client::reopen()
{
    Q_D(QAMQP::Client);
    d->disconnect();
    d->connect();
}

void QAMQP::Client::setAuth(Authenticator *auth)
{
    Q_D(QAMQP::Client);
    d->setAuth(auth);
}

Authenticator *QAMQP::Client::auth() const
{
    Q_D(const QAMQP::Client);
    return d->auth_.data();
}

bool QAMQP::Client::isSsl() const
{
    Q_D(const QAMQP::Client);
    return d->network_->isSsl();
}

void QAMQP::Client::setSsl(bool value)
{
    Q_D(QAMQP::Client);
    d->network_->setSsl(value);
}

bool QAMQP::Client::autoReconnect() const
{
    Q_D(const QAMQP::Client);
    return d->network_->autoReconnect();
}

void QAMQP::Client::setAutoReconnect(bool value)
{
    Q_D(QAMQP::Client);
    d->network_->setAutoReconnect(value);
}

bool QAMQP::Client::isConnected() const
{
    Q_D(const QAMQP::Client);
    return d->connection_->isConnected();
}

void QAMQP::Client::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(QAMQP::Client);
    return d->connection_->addCustomProperty(name, value);
}

QString QAMQP::Client::customProperty(const QString &name) const
{
    Q_D(const QAMQP::Client);
    return d->connection_->customProperty(name);
}

