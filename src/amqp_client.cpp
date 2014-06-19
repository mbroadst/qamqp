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
#include <QStringList>
#include <QtEndian>

using namespace QAMQP;

ClientPrivate::ClientPrivate(Client *q)
    : port(AMQP_PORT),
      host(QString::fromLatin1(AMQP_HOST)),
      virtualHost(QString::fromLatin1(AMQP_VHOST)),
      socket(0),
      closed(false),
      connected(false),
      channelMax(0),
      heartbeatDelay(0),
      frameMax(AMQP_FRAME_MAX),
      error(QAMQP::NoError),
      q_ptr(q)
{
}

ClientPrivate::~ClientPrivate()
{
}

void ClientPrivate::init(const QUrl &connectionString)
{
    Q_Q(Client);
    initSocket();
    heartbeatTimer = new QTimer(q);
    QObject::connect(heartbeatTimer, SIGNAL(timeout()), q, SLOT(_q_heartbeat()));

    authenticator = QSharedPointer<Authenticator>(
        new AMQPlainAuthenticator(QString::fromLatin1(AMQP_LOGIN), QString::fromLatin1(AMQP_PSWD)));

    if (connectionString.isValid()) {
        parseConnectionString(connectionString);
        _q_connect();
    }
}

void ClientPrivate::initSocket()
{
    Q_Q(Client);
    socket = new QTcpSocket(q);
    QObject::connect(socket, SIGNAL(connected()), q, SLOT(_q_socketConnected()));
    QObject::connect(socket, SIGNAL(disconnected()), q, SLOT(_q_socketDisconnected()));
    QObject::connect(socket, SIGNAL(readyRead()), q, SLOT(_q_readyRead()));
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                          q, SLOT(_q_socketError(QAbstractSocket::SocketError)));
}

void ClientPrivate::parseConnectionString(const QUrl &connectionString)
{
    Q_Q(Client);
    if (connectionString.scheme() != AMQP_SCHEME &&
        connectionString.scheme() != AMQP_SSCHEME) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid scheme: " << connectionString.scheme();
        return;
    }

    q->setPassword(connectionString.password());
    q->setUsername(connectionString.userName());
    q->setPort(connectionString.port(AMQP_PORT));
    q->setHost(connectionString.host());
    q->setVirtualHost(connectionString.path());
}

void ClientPrivate::_q_connect()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "socket already connected, disconnecting..";
        _q_disconnect();
    }

    socket->connectToHost(host, port);
}

void ClientPrivate::_q_disconnect()
{
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "already disconnected";
        return;
    }

    buffer.clear();
    close(200, "client disconnect");
}

// private slots
void ClientPrivate::_q_socketConnected()
{
    timeout = 0;
    char header[8] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    socket->write(header, 8);
}

void ClientPrivate::_q_socketDisconnected()
{
    Q_Q(Client);
    buffer.clear();
    if (connected) {
        connected = false;
        Q_EMIT q->disconnected();
    }
}

void ClientPrivate::_q_heartbeat()
{
    Frame::Heartbeat frame;
    sendFrame(frame);
}

void ClientPrivate::_q_socketError(QAbstractSocket::SocketError error)
{
    Q_Q(Client);
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
        qAmqpDebug() << "socket Error: " << socket->errorString();
        break;
    }

    // per spec, on any error we need to close the socket immediately
    // and send no more data;
    socket->close();

    if (autoReconnect) {
        QTimer::singleShot(timeout, q, SLOT(_q_connect()));
    }
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
            if (magic != Frame::FRAME_END) {
                close(UnexpectedFrameError, "wrong end of frame");
                return;
            }

            QDataStream streamB(&buffer, QIODevice::ReadOnly);
            switch(Frame::Type(type)) {
            case Frame::ftMethod:
            {
                Frame::Method frame(streamB);
                if (frame.size() > frameMax) {
                    close(FrameError, "frame size too large");
                    return;
                }

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
                if (frame.size() > frameMax) {
                    close(FrameError, "frame size too large");
                    return;
                } else if (frame.channel() <= 0) {
                    close(ChannelError, "channel number must be greater than zero");
                    return;
                }

                foreach (Frame::ContentHandler *methodHandler, contentHandlerByChannel[frame.channel()])
                    methodHandler->_q_content(frame);
            }
                break;
            case Frame::ftBody:
            {
                Frame::ContentBody frame(streamB);
                if (frame.size() > frameMax) {
                    close(FrameError, "frame size too large");
                    return;
                } else if (frame.channel() <= 0) {
                    close(ChannelError, "channel number must be greater than zero");
                    return;
                }

                foreach (Frame::ContentBodyHandler *methodHandler, bodyHandlersByChannel[frame.channel()])
                    methodHandler->_q_body(frame);
            }
                break;
            case Frame::ftHeartbeat:
            {
                Frame::Method frame(streamB);
                if (frame.channel() != 0) {
                    close(FrameError, "heartbeat must have channel id zero");
                    return;
                }

                qAmqpDebug("AMQP: Heartbeat");
            }
                break;
            default:
                qAmqpDebug() << "AMQP: Unknown frame type: " << type;
                close(FrameError, "invalid frame type");
                return;
            }
        } else {
            break;
        }
    }
}

void ClientPrivate::sendFrame(const Frame::Base &frame)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "socket not connected: " << socket->state();
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

    qAmqpDebug() << "Connection:";
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
    Q_Q(Client);
    qAmqpDebug(">> Start");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    quint8 version_major = 0;
    quint8 version_minor = 0;

    stream >> version_major >> version_minor;

    Frame::TableField table;
    Frame::deserialize(stream, table);

    QStringList mechanisms = Frame::readField('S', stream).toString().split(' ');
    QString locales = Frame::readField('S', stream).toString();

    qAmqpDebug(">> version_major: %d", version_major);
    qAmqpDebug(">> version_minor: %d", version_minor);

    Frame::print(table);

    qAmqpDebug() << ">> mechanisms: " << mechanisms;
    qAmqpDebug(">> locales: %s", qPrintable(locales));

    if (!mechanisms.contains(authenticator->type())) {
        socket->disconnectFromHost();
        Q_EMIT q->disconnected();
        return;
    }

    startOk();
}

void ClientPrivate::secure(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    qAmqpDebug() << Q_FUNC_INFO << "called!";
}

void ClientPrivate::tune(const Frame::Method &frame)
{
    qAmqpDebug(">> Tune");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    qint16 channel_max = 0,
           heartbeat_delay = 0;
    qint32 frame_max = 0;

    stream >> channel_max;
    stream >> frame_max;
    stream >> heartbeat_delay;

    if (!frameMax)
        frameMax = frame_max;
    channelMax = !channelMax ? channel_max : qMax(channel_max, channelMax);
    heartbeatDelay = !heartbeatDelay ? heartbeat_delay: qMax(heartbeat_delay, heartbeatDelay);

    qAmqpDebug(">> channel_max: %d", channelMax);
    qAmqpDebug(">> frame_max: %d", frameMax);
    qAmqpDebug(">> heartbeat: %d", heartbeatDelay);

    if (heartbeatTimer) {
        heartbeatTimer->setInterval(heartbeatDelay * 1000);
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
    qAmqpDebug(">> OpenOK");
    connected = true;
    Q_EMIT q->connected();
}

void ClientPrivate::closeOk(const Frame::Method &frame)
{
    Q_Q(Client);
    Q_UNUSED(frame)
    qAmqpDebug() << Q_FUNC_INFO << "received";
    connected = false;
    if (heartbeatTimer)
        heartbeatTimer->stop();
    Q_EMIT q->disconnected();
}

void ClientPrivate::close(const Frame::Method &frame)
{
    Q_Q(Client);
    qAmqpDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text(Frame::readField('s', stream).toString());
    stream >> classId;
    stream >> methodId;

    Error checkError = static_cast<Error>(code);
    if (checkError != QAMQP::NoError) {
        error = checkError;
        errorString = qPrintable(text);
        Q_EMIT q->error(error);
    }

    qAmqpDebug(">> code: %d", code);
    qAmqpDebug(">> text: %s", qPrintable(text));
    qAmqpDebug(">> class-id: %d", classId);
    qAmqpDebug(">> method-id: %d", methodId);
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
    qAmqpDebug() << Q_FUNC_INFO;
}

void ClientPrivate::tuneOk()
{
    Frame::Method frame(Frame::fcConnection, ClientPrivate::miTuneOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(channelMax);
    stream << qint32(frameMax);
    stream << qint16(heartbeatDelay / 1000);

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

Client::Client(ClientPrivate *dd, QObject *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

Client::~Client()
{
    Q_D(Client);
    if (d->connected)
        d->_q_disconnect();
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

QString Client::username() const
{
    Q_D(const Client);
    const Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const AMQPlainAuthenticator *a = static_cast<const AMQPlainAuthenticator*>(auth);
        return a->login();
    }

    return QString();
}

void Client::setUsername(const QString &username)
{
    Q_D(const Client);
    Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        AMQPlainAuthenticator *a = static_cast<AMQPlainAuthenticator*>(auth);
        a->setLogin(username);
    }
}

QString Client::password() const
{
    Q_D(const Client);
    const Authenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
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

qint16 Client::channelMax() const
{
    Q_D(const Client);
    return d->channelMax;
}

void Client::setChannelMax(qint16 channelMax)
{
    Q_D(Client);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->channelMax = channelMax;
}

qint32 Client::frameMax() const
{
    Q_D(const Client);
    return d->frameMax;
}

void Client::setFrameMax(qint32 frameMax)
{
    Q_D(Client);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->frameMax = qMax(frameMax, AMQP_FRAME_MIN_SIZE);
}

qint16 Client::heartbeatDelay() const
{
    Q_D(const Client);
    return d->heartbeatDelay;
}

void Client::setHeartbeatDelay(qint16 delay)
{
    Q_D(Client);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->heartbeatDelay = delay;
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

Error Client::error() const
{
    Q_D(const Client);
    return d->error;
}

QString Client::errorString() const
{
    Q_D(const Client);
    return d->errorString;
}

void Client::connectToHost(const QString &connectionString)
{
    Q_D(Client);
    if (connectionString.isEmpty()) {
        d->_q_connect();
        return;
    }

    d->parseConnectionString(QUrl::fromUserInput(connectionString));
    d->_q_connect();
}

void Client::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(Client);
    d->host = address.toString();
    d->port = port;
    d->_q_connect();
}

void Client::disconnectFromHost()
{
    Q_D(Client);
    d->_q_disconnect();
}

//////////////////////////////////////////////////////////////////////////

#ifndef QT_NO_SSL
#include <QSslSocket>

SslClientPrivate::SslClientPrivate(SslClient *q)
    : ClientPrivate(q)
{
}

void SslClientPrivate::initSocket()
{
    Q_Q(Client);
    QSslSocket *sslSocket = new QSslSocket(q);
    QObject::connect(sslSocket, SIGNAL(connected()), q, SLOT(_q_socketConnected()));
    QObject::connect(sslSocket, SIGNAL(disconnected()), q, SLOT(_q_socketDisconnected()));
    QObject::connect(sslSocket, SIGNAL(readyRead()), q, SLOT(_q_readyRead()));
    QObject::connect(sslSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                          q, SLOT(_q_socketError(QAbstractSocket::SocketError)));
    QObject::connect(sslSocket, SIGNAL(sslErrors(QList<QSslError>)),
                             q, SLOT(_q_sslErrors(QList<QSslError>)));
    socket = sslSocket;
}

void SslClientPrivate::_q_connect()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "socket already connected, disconnecting..";
        _q_disconnect();
    }

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
    if (!sslConfiguration.isNull())
        sslSocket->setSslConfiguration(sslConfiguration);
    sslSocket->connectToHostEncrypted(host, port);
}

void SslClientPrivate::_q_sslErrors(const QList<QSslError> &errors)
{
    // TODO: these need to be passed on to the user potentially, this is
    //       very unsafe
    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
    sslSocket->ignoreSslErrors(errors);
}

SslClient::SslClient(QObject *parent)
    : Client(new SslClientPrivate(this), parent)
{
}

SslClient::SslClient(const QUrl &connectionString, QObject *parent)
    : Client(new SslClientPrivate(this), parent)
{
    Q_D(SslClient);
    d->init(connectionString);
}

SslClient::~SslClient()
{
}

#endif

#include "moc_amqp_client.cpp"
