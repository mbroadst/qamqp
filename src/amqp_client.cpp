#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_global.h"
#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_authenticator.h"

#include <QTimer>
#include <QTcpSocket>
#include <QTextStream>
#include <QtEndian>

using namespace QAMQP;

ClientPrivate::ClientPrivate(Client *q)
    : port(AMQPPORT),
      host(QString::fromLatin1(AMQPHOST)),
      virtualHost(QString::fromLatin1(AMQPVHOST)),
      socket(0),
      closed(false),
      connected(false),
      q_ptr(q)
{
}

ClientPrivate::~ClientPrivate()
{
}

void ClientPrivate::init(const QUrl &connectionString)
{
    Q_Q(Client);
    socket = new QTcpSocket(q);
    QObject::connect(socket, SIGNAL(connected()), q, SLOT(_q_socketConnected()));
    QObject::connect(socket, SIGNAL(readyRead()), q, SLOT(_q_readyRead()));
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                          q, SLOT(_q_socketError(QAbstractSocket::SocketError)));

    heartbeatTimer = new QTimer(q);
    QObject::connect(heartbeatTimer, SIGNAL(timeout()), q, SLOT(_q_heartbeat()));

    authenticator = QSharedPointer<Authenticator>(
        new AMQPlainAuthenticator(QString::fromLatin1(AMQPLOGIN), QString::fromLatin1(AMQPPSWD)));

    if (connectionString.isValid()) {
        parseConnectionString(connectionString);
        connect();
    }
}

void ClientPrivate::parseConnectionString(const QUrl &connectionString)
{
    Q_Q(Client);
    if (connectionString.scheme() != AMQPSCHEME &&
        connectionString.scheme() != AMQPSSCHEME) {
        qDebug() << Q_FUNC_INFO << "invalid scheme: " << connectionString.scheme();
        return;
    }

    q->setPassword(connectionString.password());
    q->setUser(connectionString.userName());
    q->setPort(connectionString.port(AMQPPORT));
    q->setHost(connectionString.host());
    q->setVirtualHost(connectionString.path());
}

void ClientPrivate::connect()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << Q_FUNC_INFO << "socket already connected, disconnecting..";
        disconnect();
    }

    socket->connectToHost(host, port);
}

void ClientPrivate::disconnect()
{
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << Q_FUNC_INFO << "already disconnected";
        return;
    }

    close(200, "client.disconnect");

    // NOTE: this should be handled by signals, no need for dptr
    //       access here.
    // connection_->d_func()->connected = false;
}

// private slots
void ClientPrivate::_q_socketConnected()
{
    timeout = 0;
    char header[8] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    socket->write(header, 8);
}

void ClientPrivate::_q_heartbeat()
{
    Frame::Heartbeat frame;
    sendFrame(frame);
}

void ClientPrivate::_q_socketError(QAbstractSocket::SocketError error)
{
    if (timeout == 0) {
        timeout = 1000;
    } else {
        if (timeout < 120000)
            timeout *= 5;
    }

    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:

    default:
        qWarning() << "AMQP: Socket Error: " << socket->errorString();
        break;
    }

//    if (autoReconnect && connect)
//        QTimer::singleShot(timeout, this, SLOT(connectTo()));
}

void ClientPrivate::_q_readyRead()
{
    while (socket->bytesAvailable() >= Frame::HEADER_SIZE) {
        char *headerData = buffer.data();
        socket->peek(headerData, Frame::HEADER_SIZE);
        const quint32 payloadSize = qFromBigEndian<quint32>(*(quint32*)&headerData[3]);
        const qint64 readSize = Frame::HEADER_SIZE + payloadSize + Frame::FRAME_END_SIZE;

        if (socket->bytesAvailable() >= readSize) {
            buffer.resize(readSize);
            socket->read(buffer.data(), readSize);
            const char *bufferData = buffer.constData();
            const quint8 type = *(quint8*)&bufferData[0];
            const quint8 magic = *(quint8*)&bufferData[Frame::HEADER_SIZE + payloadSize];
            if (magic != Frame::FRAME_END)
                qWarning() << "Wrong end frame";

            QDataStream streamB(&buffer, QIODevice::ReadOnly);
            switch(Frame::Type(type)) {
            case Frame::ftMethod:
            {
                Frame::Method frame(streamB);
                if (frame.methodClass() == Frame::fcConnection) {
                    _q_method(frame);
                } else {
                    foreach (Frame::MethodHandler *methodHandler, methodHandlersByChannel[frame.channel()])
                        methodHandler->_q_method(frame);
                }
            }
                break;
            case Frame::ftHeader:
            {
                Frame::Content frame(streamB);
                foreach (Frame::ContentHandler *methodHandler, contentHandlerByChannel[frame.channel()])
                    methodHandler->_q_content(frame);
            }
                break;
            case Frame::ftBody:
            {
                Frame::ContentBody frame(streamB);
                foreach (Frame::ContentBodyHandler *methodHandler, bodyHandlersByChannel[frame.channel()])
                    methodHandler->_q_body(frame);
            }
                break;
            case Frame::ftHeartbeat:
                qDebug("AMQP: Heartbeat");
                break;
            default:
                qWarning() << "AMQP: Unknown frame type: " << type;
            }
        } else {
            break;
        }
    }
}

void ClientPrivate::sendFrame(const Frame::Base &frame)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << Q_FUNC_INFO << "socket not connected: " << socket->state();
        return;
    }

    QDataStream stream(socket);
    frame.toStream(stream);
}

bool ClientPrivate::_q_method(const Frame::Method &frame)
{
    Q_ASSERT(frame.methodClass() == Frame::fcConnection);
    if (frame.methodClass() != Frame::fcConnection)
        return false;

    qDebug() << "Connection:";
    if (closed) {
        if (frame.id() == ClientPrivate::miCloseOk)
            closeOk(frame);
        return false;
    }

    switch (ClientPrivate::MethodId(frame.id())) {
    case ClientPrivate::miStart:
        start(frame);
        break;
    case ClientPrivate::miSecure:
        secure(frame);
        break;
    case ClientPrivate::miTune:
        tune(frame);
        break;
    case ClientPrivate::miOpenOk:
        openOk(frame);
        break;
    case ClientPrivate::miClose:
        close(frame);
        break;
    case ClientPrivate::miCloseOk:
        closeOk(frame);
        break;
    default:
        qWarning("Unknown method-id %d", frame.id());
    }

    return true;
}

void ClientPrivate::start(const Frame::Method &frame)
{
    qDebug(">> Start");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    quint8 version_major = 0;
    quint8 version_minor = 0;

    stream >> version_major >> version_minor;

    Frame::TableField table;
    Frame::deserialize(stream, table);

    QString mechanisms = Frame::readField('S', stream).toString();
    QString locales = Frame::readField('S', stream).toString();

    qDebug(">> version_major: %d", version_major);
    qDebug(">> version_minor: %d", version_minor);

    Frame::print(table);

    qDebug(">> mechanisms: %s", qPrintable(mechanisms));
    qDebug(">> locales: %s", qPrintable(locales));

    startOk();
}

void ClientPrivate::secure(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    qDebug() << Q_FUNC_INFO << "called!";
}

void ClientPrivate::tune(const Frame::Method &frame)
{
    qDebug(">> Tune");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    qint16 channel_max = 0,
           heartbeat = 0;
    qint32 frame_max = 0;

    stream >> channel_max;
    stream >> frame_max;
    stream >> heartbeat;

    qDebug(">> channel_max: %d", channel_max);
    qDebug(">> frame_max: %d", frame_max);
    qDebug(">> heartbeat: %d", heartbeat);

    if (heartbeatTimer) {
        heartbeatTimer->setInterval(heartbeat * 1000);
        if (heartbeatTimer->interval())
            heartbeatTimer->start();
        else
            heartbeatTimer->stop();
    }

    tuneOk();
    open();
}

void ClientPrivate::openOk(const Frame::Method &frame)
{
    Q_Q(Client);
    Q_UNUSED(frame)
    qDebug(">> OpenOK");
    connected = true;
    Q_EMIT q->connected();
}

void ClientPrivate::closeOk(const Frame::Method &frame)
{
    Q_Q(Client);
    Q_UNUSED(frame)
    qDebug() << Q_FUNC_INFO << "received";
    connected = false;
    if (heartbeatTimer)
        heartbeatTimer->stop();
    Q_EMIT q->disconnected();
}

void ClientPrivate::close(const Frame::Method &frame)
{
    Q_Q(Client);
    qDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text(Frame::readField('s', stream).toString());
    stream >> classId;
    stream >> methodId;

    qDebug(">> code: %d", code);
    qDebug(">> text: %s", qPrintable(text));
    qDebug(">> class-id: %d", classId);
    qDebug(">> method-id: %d", methodId);
    connected = false;
    Q_EMIT q->disconnected();
}

void ClientPrivate::startOk()
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miStartOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::TableField clientProperties;
    clientProperties["version"] = QString(QAMQP_VERSION);
    clientProperties["platform"] = QString("Qt %1").arg(qVersion());
    clientProperties["product"] = QString("QAMQP");
    clientProperties.unite(customProperties);
    Frame::serialize(stream, clientProperties);

    authenticator->write(stream);
    Frame::writeField('s', stream, "en_US");

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ClientPrivate::secureOk()
{
    qDebug() << Q_FUNC_INFO;
}

void ClientPrivate::tuneOk()
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miTuneOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0); //channel_max
    stream << qint32(FRAME_MAX); //frame_max
    stream << qint16(heartbeatTimer->interval() / 1000); //heartbeat

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ClientPrivate::open()
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miOpen);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, virtualHost);

    stream << qint8(0);
    stream << qint8(0);

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ClientPrivate::close(int code, const QString &text, int classId, int methodId)
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miClose);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(code);
    Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ClientPrivate::closeOk()
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miCloseOk);
    connected = false;
    sendFrame(frame);
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
    Q_D(Client);
    if (d->connected)
        d->disconnect();
}

bool Client::isConnected() const
{
    Q_D(const Client);
    return d->connected;
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
    const Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const AMQPlainAuthenticator *a = static_cast<const AMQPlainAuthenticator*>(auth);
        return a->login();
    }

    return QString();
}

void Client::setUser(const QString &user)
{
    Q_D(const Client);
    Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator *a = static_cast<AMQPlainAuthenticator*>(auth);
        a->setLogin(user);
    }
}

QString Client::password() const
{
    Q_D(const Client);
    const Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == "AMQPLAIN") {
        const AMQPlainAuthenticator *a = static_cast<const AMQPlainAuthenticator*>(auth);
        return a->password();
    }

    return QString();
}

void Client::setPassword(const QString &password)
{
    Q_D(Client);
    Authenticator *auth = d->authenticator.data();
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
    d->methodHandlersByChannel[exchange->channelNumber()].append(exchange->d_func());
    connect(this, SIGNAL(connected()), exchange, SLOT(_q_open()));
    connect(this, SIGNAL(disconnected()), exchange, SLOT(_q_disconnected()));
    exchange->d_func()->open();

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
    d->methodHandlersByChannel[queue->channelNumber()].append(queue->d_func());
    d->contentHandlerByChannel[queue->channelNumber()].append(queue->d_func());
    d->bodyHandlersByChannel[queue->channelNumber()].append(queue->d_func());
    connect(this, SIGNAL(connected()), queue, SLOT(_q_open()));
    connect(this, SIGNAL(disconnected()), queue, SLOT(_q_disconnected()));
    queue->d_func()->open();

    if (!name.isEmpty())
        queue->setName(name);
    return queue;
}

void Client::setAuth(Authenticator *authenticator)
{
    Q_D(Client);
    d->authenticator = QSharedPointer<Authenticator>(authenticator);
}

Authenticator *Client::auth() const
{
    Q_D(const Client);
    return d->authenticator.data();
}

bool Client::autoReconnect() const
{
    Q_D(const Client);
    return d->autoReconnect;
}

void Client::setAutoReconnect(bool value)
{
    Q_D(Client);
    d->autoReconnect = value;
}

void Client::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(Client);
    d->customProperties.insert(name, value);
}

QString Client::customProperty(const QString &name) const
{
    Q_D(const Client);
    return d->customProperties.value(name).toString();
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

#include "moc_amqp_client.cpp"
