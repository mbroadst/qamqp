#include <QTimer>
#include <QTextStream>
#include <QStringList>
#include <QSslSocket>
#include <QtEndian>

#include "qamqpglobal.h"
#include "qamqpexchange.h"
#include "qamqpexchange_p.h"
#include "qamqpqueue.h"
#include "qamqpqueue_p.h"
#include "qamqpauthenticator.h"
#include "qamqptable.h"
#include "qamqpclient_p.h"
#include "qamqpclient.h"

QAmqpClientPrivate::QAmqpClientPrivate(QAmqpClient *q)
    : port(AMQP_PORT),
      host(AMQP_HOST),
      virtualHost(AMQP_VHOST),
      autoReconnect(false),
      timeout(0),
      connecting(false),
      useSsl(false),
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

QAmqpClientPrivate::~QAmqpClientPrivate()
{
}

void QAmqpClientPrivate::init()
{
    Q_Q(QAmqpClient);
    initSocket();
    heartbeatTimer = new QTimer(q);
    QObject::connect(heartbeatTimer, SIGNAL(timeout()), q, SLOT(_q_heartbeat()));

    authenticator = QSharedPointer<QAmqpAuthenticator>(
        new QAmqpPlainAuthenticator(QString::fromLatin1(AMQP_LOGIN), QString::fromLatin1(AMQP_PSWD)));
}

void QAmqpClientPrivate::initSocket()
{
    Q_Q(QAmqpClient);
    socket = new QSslSocket(q);
    QObject::connect(socket, SIGNAL(connected()), q, SLOT(_q_socketConnected()));
    QObject::connect(socket, SIGNAL(disconnected()), q, SLOT(_q_socketDisconnected()));
    QObject::connect(socket, SIGNAL(readyRead()), q, SLOT(_q_readyRead()));
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                          q, SLOT(_q_socketError(QAbstractSocket::SocketError)));
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                          q, SIGNAL(socketError(QAbstractSocket::SocketError)));
    QObject::connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
                          q, SIGNAL(sslErrors(QList<QSslError>)));
}

void QAmqpClientPrivate::setUsername(const QString &username)
{
    QAmqpAuthenticator *auth = authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        QAmqpPlainAuthenticator *a = static_cast<QAmqpPlainAuthenticator*>(auth);
        a->setLogin(username);
    }
}

void QAmqpClientPrivate::setPassword(const QString &password)
{
    QAmqpAuthenticator *auth = authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        QAmqpPlainAuthenticator *a = static_cast<QAmqpPlainAuthenticator*>(auth);
        a->setPassword(password);
    }
}

void QAmqpClientPrivate::parseConnectionString(const QString &uri)
{
#if QT_VERSION > 0x040801
    QUrl connectionString = QUrl::fromUserInput(uri);
#else
    QUrl connectionString(uri, QUrl::TolerantMode);
#endif

    if (connectionString.scheme() != AMQP_SCHEME &&
        connectionString.scheme() != AMQP_SSL_SCHEME) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid scheme: " << connectionString.scheme();
        return;
    }

    useSsl = (connectionString.scheme() == AMQP_SSL_SCHEME);
    port = connectionString.port((useSsl ? AMQP_SSL_PORT : AMQP_PORT));
    host = connectionString.host();

    QString vhost = connectionString.path();
    if (vhost.startsWith("/") && vhost.size() > 1)
        vhost = vhost.mid(1);
#if QT_VERSION <= 0x050200
    virtualHost = QUrl::fromPercentEncoding(vhost.toUtf8());
    setPassword(QUrl::fromPercentEncoding(connectionString.password().toUtf8()));
    setUsername(QUrl::fromPercentEncoding(connectionString.userName().toUtf8()));
#else
    virtualHost = vhost;
    setPassword(connectionString.password());
    setUsername(connectionString.userName());
#endif
}

void QAmqpClientPrivate::_q_connect()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "socket already connected, disconnecting..";
        _q_disconnect();
    }

    qAmqpDebug() << "connecting to host: " << host << ", port: " << port;
    if (useSsl)
        socket->connectToHostEncrypted(host, port);
    else
        socket->connectToHost(host, port);
}

void QAmqpClientPrivate::_q_disconnect()
{
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "already disconnected";
        return;
    }

    buffer.clear();
    close(200, "client disconnect");
}

// private slots
void QAmqpClientPrivate::_q_socketConnected()
{
    timeout = 0;
    char header[8] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    socket->write(header, 8);
}

void QAmqpClientPrivate::_q_socketDisconnected()
{
    Q_Q(QAmqpClient);
    buffer.clear();
    if (connected) {
        connected = false;
        Q_EMIT q->disconnected();
    }
}

void QAmqpClientPrivate::_q_heartbeat()
{
    QAmqpHeartbeatFrame frame;
    sendFrame(frame);
}

void QAmqpClientPrivate::_q_socketError(QAbstractSocket::SocketError error)
{
    Q_Q(QAmqpClient);
    if (timeout <= 0) {
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
        qAmqpDebug() << "socket error: " << socket->errorString();
        break;
    }

    // per spec, on any error we need to close the socket immediately
    // and send no more data;
    socket->close();
    errorString = socket->errorString();

    if (autoReconnect) {
        qAmqpDebug() << "trying to reconnect after: " << timeout << "ms";
        QTimer::singleShot(timeout, q, SLOT(_q_connect()));
    }
}

void QAmqpClientPrivate::_q_readyRead()
{
    while (socket->bytesAvailable() >= QAmqpFrame::HEADER_SIZE) {
        unsigned char headerData[QAmqpFrame::HEADER_SIZE];
        socket->peek((char*)headerData, QAmqpFrame::HEADER_SIZE);
        const quint32 payloadSize = qFromBigEndian<quint32>(headerData + 3);
        const qint64 readSize = QAmqpFrame::HEADER_SIZE + payloadSize + QAmqpFrame::FRAME_END_SIZE;

        if (socket->bytesAvailable() < readSize)
            return;

        buffer.resize(readSize);
        socket->read(buffer.data(), readSize);
        const char *bufferData = buffer.constData();
        const quint8 type = *(quint8*)&bufferData[0];
        const quint8 magic = *(quint8*)&bufferData[QAmqpFrame::HEADER_SIZE + payloadSize];
        if (Q_UNLIKELY(magic != QAmqpFrame::FRAME_END)) {
            close(QAMQP::UnexpectedFrameError, "wrong end of frame");
            return;
        }

        QDataStream streamB(&buffer, QIODevice::ReadOnly);
        switch (static_cast<QAmqpFrame::FrameType>(type)) {
        case QAmqpFrame::Method:
        {
            QAmqpMethodFrame frame;
            streamB >> frame;

            if (Q_UNLIKELY(frame.size() > frameMax)) {
                close(QAMQP::FrameError, "frame size too large");
                return;
            }

            if (frame.methodClass() == QAmqpFrame::Connection) {
                _q_method(frame);
            } else {
                foreach (QAmqpMethodFrameHandler *methodHandler, methodHandlersByChannel[frame.channel()])
                    methodHandler->_q_method(frame);
            }
        }
            break;
        case QAmqpFrame::Header:
        {
            QAmqpContentFrame frame;
            streamB >> frame;

            if (Q_UNLIKELY(frame.size() > frameMax)) {
                close(QAMQP::FrameError, "frame size too large");
                return;
            } else if (Q_UNLIKELY(frame.channel() <= 0)) {
                close(QAMQP::ChannelError, "channel number must be greater than zero");
                return;
            }

            foreach (QAmqpContentFrameHandler *methodHandler, contentHandlerByChannel[frame.channel()])
                methodHandler->_q_content(frame);
        }
            break;
        case QAmqpFrame::Body:
        {
            QAmqpContentBodyFrame frame;
            streamB >> frame;

            if (Q_UNLIKELY(frame.size() > frameMax)) {
                close(QAMQP::FrameError, "frame size too large");
                return;
            } else if (Q_UNLIKELY(frame.channel() <= 0)) {
                close(QAMQP::ChannelError, "channel number must be greater than zero");
                return;
            }

            foreach (QAmqpContentBodyFrameHandler *methodHandler, bodyHandlersByChannel[frame.channel()])
                methodHandler->_q_body(frame);
        }
            break;
        case QAmqpFrame::Heartbeat:
        {
            QAmqpMethodFrame frame;
            streamB >> frame;

            if (Q_UNLIKELY(frame.channel() != 0)) {
                close(QAMQP::FrameError, "heartbeat must have channel id zero");
                return;
            }

            qAmqpDebug("AMQP: Heartbeat");
        }
            break;
        default:
            qAmqpDebug() << "AMQP: Unknown frame type: " << type;
            close(QAMQP::FrameError, "invalid frame type");
            return;
        }
    }
}

void QAmqpClientPrivate::sendFrame(const QAmqpFrame &frame)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qAmqpDebug() << Q_FUNC_INFO << "socket not connected: " << socket->state();
        return;
    }

    QDataStream stream(socket);
    stream << frame;
}

bool QAmqpClientPrivate::_q_method(const QAmqpMethodFrame &frame)
{
    Q_ASSERT(frame.methodClass() == QAmqpFrame::Connection);
    if (frame.methodClass() != QAmqpFrame::Connection)
        return false;

    qAmqpDebug() << "Connection:";
    if (closed) {
        if (frame.id() == QAmqpClientPrivate::miCloseOk)
            closeOk(frame);
        return false;
    }

    switch (QAmqpClientPrivate::MethodId(frame.id())) {
    case QAmqpClientPrivate::miStart:
        start(frame);
        break;
    case QAmqpClientPrivate::miSecure:
        secure(frame);
        break;
    case QAmqpClientPrivate::miTune:
        tune(frame);
        break;
    case QAmqpClientPrivate::miOpenOk:
        openOk(frame);
        break;
    case QAmqpClientPrivate::miClose:
        close(frame);
        break;
    case QAmqpClientPrivate::miCloseOk:
        closeOk(frame);
        break;
    default:
        qAmqpDebug("Unknown method-id %d", frame.id());
    }

    return true;
}

void QAmqpClientPrivate::start(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpClient);
    qAmqpDebug(">> Start");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);

    quint8 version_major = 0;
    quint8 version_minor = 0;
    stream >> version_major >> version_minor;

    QAmqpTable table;
    stream >> table;

    QStringList mechanisms =
        QAmqpFrame::readAmqpField(stream, QAmqpMetaType::LongString).toString().split(' ');
    QString locales = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::LongString).toString();

    qAmqpDebug(">> version_major: %d", version_major);
    qAmqpDebug(">> version_minor: %d", version_minor);

    // NOTE: replace with qDebug overload
    // QAmqpFrame::print(table);

    qAmqpDebug() << ">> mechanisms: " << mechanisms;
    qAmqpDebug(">> locales: %s", qPrintable(locales));

    if (!mechanisms.contains(authenticator->type())) {
        socket->disconnectFromHost();
        Q_EMIT q->disconnected();
        return;
    }

    startOk();
}

void QAmqpClientPrivate::secure(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame)
    qAmqpDebug() << Q_FUNC_INFO << "called!";
}

void QAmqpClientPrivate::tune(const QAmqpMethodFrame &frame)
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

void QAmqpClientPrivate::openOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpClient);
    Q_UNUSED(frame)
    qAmqpDebug(">> OpenOK");
    connected = true;
    Q_EMIT q->connected();
}

void QAmqpClientPrivate::closeOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpClient);
    Q_UNUSED(frame)
    qAmqpDebug() << Q_FUNC_INFO << "received";
    connected = false;
    if (heartbeatTimer)
        heartbeatTimer->stop();
    socket->disconnectFromHost();
    Q_EMIT q->disconnected();
}

void QAmqpClientPrivate::close(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpClient);
    qAmqpDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();
    stream >> classId;
    stream >> methodId;

    QAMQP::Error checkError = static_cast<QAMQP::Error>(code);
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

    // complete handshake
    QAmqpMethodFrame closeOkFrame(QAmqpFrame::Connection, QAmqpClientPrivate::miCloseOk);
    sendFrame(closeOkFrame);
}

void QAmqpClientPrivate::startOk()
{
    QAmqpMethodFrame frame(QAmqpFrame::Connection, QAmqpClientPrivate::miStartOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    QAmqpTable clientProperties;
    clientProperties["version"] = QString(QAMQP_VERSION);
    clientProperties["platform"] = QString("Qt %1").arg(qVersion());
    clientProperties["product"] = QString("QAMQP");
    clientProperties.unite(customProperties);
    stream << clientProperties;

    authenticator->write(stream);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, QLatin1String("en_US"));

    frame.setArguments(arguments);
    sendFrame(frame);
}

void QAmqpClientPrivate::secureOk()
{
    qAmqpDebug() << Q_FUNC_INFO;
}

void QAmqpClientPrivate::tuneOk()
{
    QAmqpMethodFrame frame(QAmqpFrame::Connection, QAmqpClientPrivate::miTuneOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(channelMax);
    stream << qint32(frameMax);
    stream << qint16(heartbeatDelay);

    frame.setArguments(arguments);
    sendFrame(frame);
}

void QAmqpClientPrivate::open()
{
    QAmqpMethodFrame frame(QAmqpFrame::Connection, QAmqpClientPrivate::miOpen);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, virtualHost);

    stream << qint8(0);
    stream << qint8(0);

    frame.setArguments(arguments);
    sendFrame(frame);
}

void QAmqpClientPrivate::close(int code, const QString &text, int classId, int methodId)
{
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);
    stream << qint16(code);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    QAmqpMethodFrame frame(QAmqpFrame::Connection, QAmqpClientPrivate::miClose);
    frame.setArguments(arguments);
    sendFrame(frame);
}

//////////////////////////////////////////////////////////////////////////

QAmqpClient::QAmqpClient(QObject *parent)
    : QObject(parent),
      d_ptr(new QAmqpClientPrivate(this))
{
    Q_D(QAmqpClient);
    d->init();
}

QAmqpClient::QAmqpClient(QAmqpClientPrivate *dd, QObject *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

QAmqpClient::~QAmqpClient()
{
    Q_D(QAmqpClient);
    if (d->connected)
        d->_q_disconnect();
}

bool QAmqpClient::isConnected() const
{
    Q_D(const QAmqpClient);
    return d->connected;
}

quint16 QAmqpClient::port() const
{
    Q_D(const QAmqpClient);
    return d->port;
}

void QAmqpClient::setPort(quint16 port)
{
    Q_D(QAmqpClient);
    d->port = port;
}

QString QAmqpClient::host() const
{
    Q_D(const QAmqpClient);
    return d->host;
}

void QAmqpClient::setHost(const QString &host)
{
    Q_D(QAmqpClient);
    d->host = host;
}

QString QAmqpClient::virtualHost() const
{
    Q_D(const QAmqpClient);
    return d->virtualHost;
}

void QAmqpClient::setVirtualHost(const QString &virtualHost)
{
    Q_D(QAmqpClient);
    d->virtualHost = virtualHost;
}

QString QAmqpClient::username() const
{
    Q_D(const QAmqpClient);
    const QAmqpAuthenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const QAmqpPlainAuthenticator *a = static_cast<const QAmqpPlainAuthenticator*>(auth);
        return a->login();
    }

    return QString();
}

void QAmqpClient::setUsername(const QString &username)
{
    Q_D(QAmqpClient);
    d->setUsername(username);
}

QString QAmqpClient::password() const
{
    Q_D(const QAmqpClient);
    const QAmqpAuthenticator *auth = d->authenticator.data();
    if (auth && auth->type() == QLatin1String("AMQPLAIN")) {
        const QAmqpPlainAuthenticator *a = static_cast<const QAmqpPlainAuthenticator*>(auth);
        return a->password();
    }

    return QString();
}

void QAmqpClient::setPassword(const QString &password)
{
    Q_D(QAmqpClient);
    d->setPassword(password);
}

QAmqpExchange *QAmqpClient::createExchange(int channelNumber)
{
    return createExchange(QString(), channelNumber);
}

QAmqpExchange *QAmqpClient::createExchange(const QString &name, int channelNumber)
{
    Q_D(QAmqpClient);
    QAmqpExchange *exchange = new QAmqpExchange(channelNumber, this);
    d->methodHandlersByChannel[exchange->channelNumber()].append(exchange->d_func());
    connect(this, SIGNAL(connected()), exchange, SLOT(_q_open()));
    connect(this, SIGNAL(disconnected()), exchange, SLOT(_q_disconnected()));
    exchange->d_func()->open();

    if (!name.isEmpty())
        exchange->setName(name);
    return exchange;
}

QAmqpQueue *QAmqpClient::createQueue(int channelNumber)
{
    return createQueue(QString(), channelNumber);
}

QAmqpQueue *QAmqpClient::createQueue(const QString &name, int channelNumber)
{
    Q_D(QAmqpClient);
    QAmqpQueue *queue = new QAmqpQueue(channelNumber, this);
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

void QAmqpClient::setAuth(QAmqpAuthenticator *authenticator)
{
    Q_D(QAmqpClient);
    d->authenticator = QSharedPointer<QAmqpAuthenticator>(authenticator);
}

QAmqpAuthenticator *QAmqpClient::auth() const
{
    Q_D(const QAmqpClient);
    return d->authenticator.data();
}

bool QAmqpClient::autoReconnect() const
{
    Q_D(const QAmqpClient);
    return d->autoReconnect;
}

void QAmqpClient::setAutoReconnect(bool value)
{
    Q_D(QAmqpClient);
    d->autoReconnect = value;
}

qint16 QAmqpClient::channelMax() const
{
    Q_D(const QAmqpClient);
    return d->channelMax;
}

void QAmqpClient::setChannelMax(qint16 channelMax)
{
    Q_D(QAmqpClient);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->channelMax = channelMax;
}

qint32 QAmqpClient::frameMax() const
{
    Q_D(const QAmqpClient);
    return d->frameMax;
}

void QAmqpClient::setFrameMax(qint32 frameMax)
{
    Q_D(QAmqpClient);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->frameMax = qMax(frameMax, AMQP_FRAME_MIN_SIZE);
}

qint16 QAmqpClient::heartbeatDelay() const
{
    Q_D(const QAmqpClient);
    return d->heartbeatDelay;
}

void QAmqpClient::setHeartbeatDelay(qint16 delay)
{
    Q_D(QAmqpClient);
    if (d->connected) {
        qAmqpDebug() << Q_FUNC_INFO << "can't modify value while connected";
        return;
    }

    d->heartbeatDelay = delay;
}

void QAmqpClient::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(QAmqpClient);
    d->customProperties.insert(name, value);
}

QString QAmqpClient::customProperty(const QString &name) const
{
    Q_D(const QAmqpClient);
    return d->customProperties.value(name).toString();
}

QAbstractSocket::SocketError QAmqpClient::socketError() const
{
    Q_D(const QAmqpClient);
    return d->socket->error();
}

QAMQP::Error QAmqpClient::error() const
{
    Q_D(const QAmqpClient);
    return d->error;
}

QString QAmqpClient::errorString() const
{
    Q_D(const QAmqpClient);
    return d->errorString;
}

QSslConfiguration QAmqpClient::sslConfiguration() const
{
    Q_D(const QAmqpClient);
    return d->socket->sslConfiguration();
}

void QAmqpClient::setSslConfiguration(const QSslConfiguration &config)
{
    Q_D(QAmqpClient);
    if (!config.isNull()) {
        d->useSsl = true;
        d->port = AMQP_SSL_PORT;
        d->socket->setSslConfiguration(config);
    }
}

void QAmqpClient::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_D(QAmqpClient);
    d->socket->ignoreSslErrors(errors);
}

void QAmqpClient::connectToHost(const QString &uri)
{
    Q_D(QAmqpClient);
    if (uri.isEmpty()) {
        d->_q_connect();
        return;
    }

    d->parseConnectionString(uri);
    d->_q_connect();
}

void QAmqpClient::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(QAmqpClient);
    d->host = address.toString();
    d->port = port;
    d->_q_connect();
}

void QAmqpClient::disconnectFromHost()
{
    Q_D(QAmqpClient);
    d->_q_disconnect();
}

#include "moc_qamqpclient.cpp"
