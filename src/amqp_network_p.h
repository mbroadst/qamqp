#ifndef amqp_network_h__
#define amqp_network_h__

#include <QObject>
#include <QTcpSocket>
#ifndef QT_NO_SSL
#   include <QSslSocket>
#endif
#include <QPointer>
#include <QBuffer>

#include "amqp_frame.h"
#include "amqp_global.h"

namespace QAMQP
{

class NetworkPrivate;
class QAMQP_EXPORT Network : public QObject
{
    Q_OBJECT
public:
    Network(QObject *parent = 0);
    ~Network();

    void disconnect();
    void sendFrame(const Frame::Base &frame);

    bool isSsl() const;
    void setSsl(bool value);

    bool autoReconnect() const;
    void setAutoReconnect(bool value);

    QAbstractSocket::SocketState state() const;

    typedef qint16 Channel;
    void setMethodHandlerConnection(Frame::MethodHandler *pMethodHandlerConnection);
    void addMethodHandlerForChannel(Channel channel, Frame::MethodHandler *pHandler);
    void addContentHandlerForChannel(Channel channel, Frame::ContentHandler *pHandler);
    void addContentBodyHandlerForChannel(Channel channel, Frame::ContentBodyHandler *pHandler);

Q_SIGNALS:
    void connected();
    void disconnected();

public Q_SLOTS:
    void connectTo(const QString &host = QString(), quint16 port = 0);
    void error(QAbstractSocket::SocketError socketError);

private Q_SLOTS:
    void readyRead();
    void sslErrors();
    void conectionReady();

private:
    Q_DISABLE_COPY(Network)
    Q_DECLARE_PRIVATE(Network)
    QScopedPointer<NetworkPrivate> d_ptr;

};

} // namespace QAMQP

#endif // amqp_network_h__
