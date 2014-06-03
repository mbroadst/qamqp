#include "amqp_network.h"
#include <QDebug>
#include <QTimer>
#include <QtEndian>

using namespace QAMQP;

Network::Network(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Frame::Method>("QAMQP::Frame::Method");

    buffer_.reserve(Frame::HEADER_SIZE);
    timeOut_ = 1000;
    connect_ = false;

    initSocket(false);
}

Network::~Network()
{
    disconnect();
}

void Network::connectTo(const QString &host, quint16 port)
{
    if (!socket_) {
        qWarning("AMQP: Socket didn't create.");
        return;
    }

    QString h(host);
    int p(port);
    connect_ = true;
    if (host.isEmpty())
        h = lastHost_ ;
    if (port == 0)
        p = lastPort_;

    if (isSsl()) {
#ifndef QT_NO_SSL
        static_cast<QSslSocket*>(socket_.data())->connectToHostEncrypted(h, p);
#else
        qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
    } else {
        socket_->connectToHost(h, p);
    }

    lastHost_ = h;
    lastPort_ = p;
}

void Network::disconnect()
{
    connect_ = false;
    if (socket_)
        socket_->close();
}

void Network::error(QAbstractSocket::SocketError socketError)
{
    if (timeOut_ == 0) {
        timeOut_ = 1000;
    } else {
        if (timeOut_ < 120000)
            timeOut_ *= 5;
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
        qWarning() << "AMQP: Socket Error: " << socket_->errorString();
        break;
    }

    if (autoReconnect_ && connect_)
        QTimer::singleShot(timeOut_, this, SLOT(connectTo()));
}

void Network::readyRead()
{
    while (socket_->bytesAvailable() >= Frame::HEADER_SIZE) {
        char *headerData = buffer_.data();
        socket_->peek(headerData, Frame::HEADER_SIZE);
        const quint32 payloadSize = qFromBigEndian<quint32>(*(quint32*)&headerData[3]);
        const qint64 readSize = Frame::HEADER_SIZE+payloadSize + Frame::FRAME_END_SIZE;

        if (socket_->bytesAvailable() >= readSize) {
            buffer_.resize(readSize);
            socket_->read(buffer_.data(), readSize);
            const char *bufferData = buffer_.constData();
            const quint8 type = *(quint8*)&bufferData[0];
            const quint8 magic = *(quint8*)&bufferData[Frame::HEADER_SIZE + payloadSize];
            if (magic != Frame::FRAME_END)
                qWarning() << "Wrong end frame";

            QDataStream streamB(&buffer_, QIODevice::ReadOnly);
            switch(Frame::Type(type)) {
            case Frame::ftMethod:
            {
                Frame::Method frame(streamB);
                if (frame.methodClass() == Frame::fcConnection) {
                    m_pMethodHandlerConnection->_q_method(frame);
                } else {
                    foreach (Frame::MethodHandler *pMethodHandler, m_methodHandlersByChannel[frame.channel()])
                        pMethodHandler->_q_method(frame);
                }
            }
                break;
            case Frame::ftHeader:
            {
                Frame::Content frame(streamB);
                foreach (Frame::ContentHandler *pMethodHandler, m_contentHandlerByChannel[frame.channel()])
                    pMethodHandler->_q_content(frame);
            }
                break;
            case Frame::ftBody:
            {
                Frame::ContentBody frame(streamB);
                foreach (Frame::ContentBodyHandler *pMethodHandler, m_bodyHandlersByChannel[frame.channel()])
                    pMethodHandler->_q_body(frame);
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
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        qDebug() << Q_FUNC_INFO << "socket not connected: " << socket_->state();
        return;
    }

    QDataStream stream(socket_);
    frame.toStream(stream);
}

bool Network::isSsl() const
{
    if (socket_)
        return QString(socket_->metaObject()->className()).compare("QSslSocket", Qt::CaseInsensitive) == 0;
    return false;
}

void Network::setSsl(bool value)
{
    initSocket(value);
}

void Network::initSocket(bool ssl)
{
    if (socket_) {
        socket_->deleteLater();
        socket_ = 0;
    }

    if (ssl) {
#ifndef QT_NO_SSL
        socket_ = new QSslSocket(this);
        QSslSocket *ssl_= static_cast<QSslSocket*> (socket_.data());
        ssl_->setProtocol(QSsl::AnyProtocol);
        connect(socket_, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors()));
        connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
#else
        qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
    } else {
        socket_ = new QTcpSocket(this);
        connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
    }

    if (socket_) {
        connect(socket_, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
        connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
        connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)),
                   this, SLOT(error(QAbstractSocket::SocketError)));
    }
}

void Network::sslErrors()
{
#ifndef QT_NO_SSL
    static_cast<QSslSocket*>(socket_.data())->ignoreSslErrors();
#endif
}

void Network::conectionReady()
{
    timeOut_ = 0;
    char header[8] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    socket_->write(header, 8);
    Q_EMIT connected();
}

bool Network::autoReconnect() const
{
    return autoReconnect_;
}

void Network::setAutoReconnect(bool value)
{
    autoReconnect_ = value;
}

QAbstractSocket::SocketState Network::state() const
{
    if (socket_)
        return socket_->state();
    return QAbstractSocket::UnconnectedState;
}

void Network::setMethodHandlerConnection(Frame::MethodHandler *pMethodHandlerConnection)
{
    m_pMethodHandlerConnection = pMethodHandlerConnection;
}

void Network::addMethodHandlerForChannel(Channel channel, Frame::MethodHandler *pHandler)
{
    m_methodHandlersByChannel[channel].append(pHandler);
}

void Network::addContentHandlerForChannel(Channel channel, Frame::ContentHandler *pHandler)
{
    m_contentHandlerByChannel[channel].append(pHandler);
}

void Network::addContentBodyHandlerForChannel(Channel channel, Frame::ContentBodyHandler *pHandler)
{
    m_bodyHandlersByChannel[channel].append(pHandler);
}
