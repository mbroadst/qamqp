#ifndef QAMQPCLIENT_P_H
#define QAMQPCLIENT_P_H

#include <QHash>
#include <QSharedPointer>
#include <QPointer>
#include <QAbstractSocket>
#include <QSslError>

#include "qamqpchannelhash_p.h"
#include "qamqpglobal.h"
#include "qamqpauthenticator.h"
#include "qamqptable.h"
#include "qamqpframe_p.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

class QTimer;
class QSslSocket;
class QAmqpClient;
class QAmqpQueue;
class QAmqpExchange;
class QAmqpConnection;
class QAmqpAuthenticator;
class QAMQP_EXPORT QAmqpClientPrivate : public QAmqpMethodFrameHandler
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miStart, 10),
        METHOD_ID_ENUM(miSecure, 20),
        METHOD_ID_ENUM(miTune, 30),
        METHOD_ID_ENUM(miOpen, 40),
        METHOD_ID_ENUM(miClose, 50)
    };

    QAmqpClientPrivate(QAmqpClient *q);
    virtual ~QAmqpClientPrivate();

    virtual void init();
    virtual void initSocket();
    void resetChannelState();
    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void parseConnectionString(const QString &uri);
    void sendFrame(const QAmqpFrame &frame);

    void closeConnection();

    // private slots
    void _q_socketConnected();
    void _q_socketDisconnected();
    void _q_readyRead();
    void _q_socketError(QAbstractSocket::SocketError error);
    void _q_heartbeat();
    virtual void _q_connect();
    void _q_disconnect();

    virtual bool _q_method(const QAmqpMethodFrame &frame);

    // method handlers, FROM server
    void start(const QAmqpMethodFrame &frame);
    void secure(const QAmqpMethodFrame &frame);
    void tune(const QAmqpMethodFrame &frame);
    void openOk(const QAmqpMethodFrame &frame);
    void closeOk(const QAmqpMethodFrame &frame);

    // method handlers, TO server
    void startOk();
    void secureOk();
    void tuneOk();
    void open();

    // method handlers, BOTH ways
    void close(int code, const QString &text, int classId = 0, int methodId = 0);
    void close(const QAmqpMethodFrame &frame);

    quint16 port;
    QString host;
    QString virtualHost;

    QSharedPointer<QAmqpAuthenticator> authenticator;

    // Network
    QByteArray buffer;
    bool autoReconnect;
    bool reconnectFixedTimeout;
    int timeout;
    bool connecting;
    bool useSsl;

    QSslSocket *socket;
    QHash<quint16, QList<QAmqpMethodFrameHandler*> > methodHandlersByChannel;
    QHash<quint16, QList<QAmqpContentFrameHandler*> > contentHandlerByChannel;
    QHash<quint16, QList<QAmqpContentBodyFrameHandler*> > bodyHandlersByChannel;

    // Connection
    bool closed;
    bool connected;
    QPointer<QTimer> heartbeatTimer;
    QPointer<QTimer> reconnectTimer;
    QAmqpTable customProperties;
    qint16 channelMax;
    qint16 heartbeatDelay;
    qint32 frameMax;

    QAMQP::Error error;
    QString errorString;

    /*! Exchange objects */
    QAmqpChannelHash exchanges;

    /*! Named queue objects */
    QAmqpChannelHash queues;

    QAmqpClient * const q_ptr;
    Q_DECLARE_PUBLIC(QAmqpClient)

};

#endif // QAMQPCLIENT_P_H
