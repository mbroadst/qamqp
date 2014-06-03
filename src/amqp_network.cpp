#include "amqp_network_p.h"

#include <QDebug>
#include <QTimer>
#include <QtEndian>

namespace QAMQP {

class NetworkPrivate {
public:
    NetworkPrivate(Network *qq);
    void initSocket(bool ssl = false);

    static int s_frameMethodMetaType;

    QPointer<QTcpSocket> socket;
    QByteArray buffer;
    QString lastHost;
    int lastPort;
    bool autoReconnect;
    int timeOut;
    bool connect;

    Frame::MethodHandler *connectionMethodHandler;
    QHash<Network::Channel, QList<Frame::MethodHandler*> > methodHandlersByChannel;
    QHash<Network::Channel, QList<Frame::ContentHandler*> > contentHandlerByChannel;
    QHash<Network::Channel, QList<Frame::ContentBodyHandler*> > bodyHandlersByChannel;

    Q_DECLARE_PUBLIC(Network)
    Network * const q_ptr;
};

int NetworkPrivate::s_frameMethodMetaType = qRegisterMetaType<Frame::Method>("QAMQP::Frame::Method");
NetworkPrivate::NetworkPrivate(Network *qq)
    : lastPort(0),
      autoReconnect(false),
      timeOut(1000),
      connect(false),
      q_ptr(qq)
{
    buffer.reserve(Frame::HEADER_SIZE);
}

void NetworkPrivate::initSocket(bool ssl)
{
    Q_Q(Network);
    if (socket) {
        socket->deleteLater();
        socket = 0;
    }

    if (ssl) {
#ifndef QT_NO_SSL
        socket = new QSslSocket(q);
        QSslSocket *sslSocket = static_cast<QSslSocket*>(socket.data());
        sslSocket->setProtocol(QSsl::AnyProtocol);
        QObject::connect(socket, SIGNAL(sslErrors(const QList<QSslError> &)), q, SLOT(sslErrors()));
        QObject::connect(socket, SIGNAL(connected()), q, SLOT(conectionReady()));
#else
        qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
    } else {
        socket = new QTcpSocket(q);
        QObject::connect(socket, SIGNAL(connected()), q, SLOT(conectionReady()));
    }

    if (socket) {
        QObject::connect(socket, SIGNAL(disconnected()), q, SIGNAL(disconnected()));
        QObject::connect(socket, SIGNAL(readyRead()), q, SLOT(readyRead()));
        QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                              q, SLOT(error(QAbstractSocket::SocketError)));
    }
}

}   // namespace QAMQP

//////////////////////////////////////////////////////////////////////////

using namespace QAMQP;
Network::Network(QObject *parent)
    : QObject(parent),
      d_ptr(new NetworkPrivate(this))
{
    Q_D(Network);
    d->initSocket(false);
}

Network::~Network()
{
    disconnect();
}

void Network::connectTo(const QString &host, quint16 port)
{
    Q_D(Network);
    if (!d->socket) {
        qWarning("AMQP: Socket didn't create.");
        return;
    }

    QString h(host);
    int p(port);
    d->connect = true;
    if (host.isEmpty())
        h = d->lastHost;
    if (port == 0)
        p = d->lastPort;

    if (isSsl()) {
#ifndef QT_NO_SSL
        static_cast<QSslSocket*>(d->socket.data())->connectToHostEncrypted(h, p);
#else
        qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
    } else {
        d->socket->connectToHost(h, p);
    }

    d->lastHost = h;
    d->lastPort = p;
}

void Network::disconnect()
{
    Q_D(Network);
    d->connect = false;
    if (d->socket)
        d->socket->close();
}

void Network::error(QAbstractSocket::SocketError socketError)
{
    Q_D(Network);
    if (d->timeOut == 0) {
        d->timeOut = 1000;
    } else {
        if (d->timeOut < 120000)
            d->timeOut *= 5;
    }

    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionRefusedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:

    default:
        qWarning() << "AMQP: Socket Error: " << d->socket->errorString();
        break;
    }

    if (d->autoReconnect && d->connect)
        QTimer::singleShot(d->timeOut, this, SLOT(connectTo()));
}

void Network::readyRead()
{
    Q_D(Network);
    while (d->socket->bytesAvailable() >= Frame::HEADER_SIZE) {
        char *headerData = d->buffer.data();
        d->socket->peek(headerData, Frame::HEADER_SIZE);
        const quint32 payloadSize = qFromBigEndian<quint32>(*(quint32*)&headerData[3]);
        const qint64 readSize = Frame::HEADER_SIZE + payloadSize + Frame::FRAME_END_SIZE;

        if (d->socket->bytesAvailable() >= readSize) {
            d->buffer.resize(readSize);
            d->socket->read(d->buffer.data(), readSize);
            const char *bufferData = d->buffer.constData();
            const quint8 type = *(quint8*)&bufferData[0];
            const quint8 magic = *(quint8*)&bufferData[Frame::HEADER_SIZE + payloadSize];
            if (magic != Frame::FRAME_END)
                qWarning() << "Wrong end frame";

            QDataStream streamB(&d->buffer, QIODevice::ReadOnly);
            switch(Frame::Type(type)) {
            case Frame::ftMethod:
            {
                Frame::Method frame(streamB);
                if (frame.methodClass() == Frame::fcConnection) {
                    d->connectionMethodHandler->_q_method(frame);
                } else {
                    foreach (Frame::MethodHandler *methodHandler, d->methodHandlersByChannel[frame.channel()])
                        methodHandler->_q_method(frame);
                }
            }
                break;
            case Frame::ftHeader:
            {
                Frame::Content frame(streamB);
                foreach (Frame::ContentHandler *methodHandler, d->contentHandlerByChannel[frame.channel()])
                    methodHandler->_q_content(frame);
            }
                break;
            case Frame::ftBody:
            {
                Frame::ContentBody frame(streamB);
                foreach (Frame::ContentBodyHandler *methodHandler, d->bodyHandlersByChannel[frame.channel()])
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

void Network::sendFrame(const Frame::Base &frame)
{
    Q_D(Network);
    if (d->socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << Q_FUNC_INFO << "socket not connected: " << d->socket->state();
        return;
    }

    QDataStream stream(d->socket);
    frame.toStream(stream);
}

bool Network::isSsl() const
{
    Q_D(const Network);
    if (d->socket)
        return QString(d->socket->metaObject()->className()).compare("QSslSocket", Qt::CaseInsensitive) == 0;
    return false;
}

void Network::setSsl(bool value)
{
    Q_D(Network);
    d->initSocket(value);
}

void Network::sslErrors()
{
#ifndef QT_NO_SSL
    Q_D(Network);
    static_cast<QSslSocket*>(d->socket.data())->ignoreSslErrors();
#endif
}

void Network::conectionReady()
{
    Q_D(Network);
    d->timeOut = 0;
    char header[8] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    d->socket->write(header, 8);
    Q_EMIT connected();
}

bool Network::autoReconnect() const
{
    Q_D(const Network);
    return d->autoReconnect;
}

void Network::setAutoReconnect(bool value)
{
    Q_D(Network);
    d->autoReconnect = value;
}

QAbstractSocket::SocketState Network::state() const
{
    Q_D(const Network);
    if (d->socket)
        return d->socket->state();
    return QAbstractSocket::UnconnectedState;
}

void Network::setMethodHandlerConnection(Frame::MethodHandler *connectionMethodHandler)
{
    Q_D(Network);
    d->connectionMethodHandler = connectionMethodHandler;
}

void Network::addMethodHandlerForChannel(Channel channel, Frame::MethodHandler *methodHandler)
{
    Q_D(Network);
    d->methodHandlersByChannel[channel].append(methodHandler);
}

void Network::addContentHandlerForChannel(Channel channel, Frame::ContentHandler *methodHandler)
{
    Q_D(Network);
    d->contentHandlerByChannel[channel].append(methodHandler);
}

void Network::addContentBodyHandlerForChannel(Channel channel, Frame::ContentBodyHandler *methodHandler)
{
    Q_D(Network);
    d->bodyHandlersByChannel[channel].append(methodHandler);
}
