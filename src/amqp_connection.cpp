#include "amqp_connection_p.h"
#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_frame.h"
#include "amqp_global.h"
#include "amqp_network_p.h"
#include "amqp_authenticator.h"

#include <QDebug>
#include <QDataStream>
#include <QTimer>

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP {

class ConnectionPrivate
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miStart, 10),
        METHOD_ID_ENUM(miSecure, 20),
        METHOD_ID_ENUM(miTune, 30),
        METHOD_ID_ENUM(miOpen, 40),
        METHOD_ID_ENUM(miClose, 50)
    };

    ConnectionPrivate(Connection *q);

    // private slots
    void _q_heartbeat();

    QPointer<Client> client;
    QPointer<Network> network;
    bool closed;
    bool connected;
    QPointer<QTimer> heartbeatTimer;
    Frame::TableField customProperty;

    Q_DECLARE_PUBLIC(Connection)
    Connection * const q_ptr;
};

ConnectionPrivate::ConnectionPrivate(Connection *q)
    : closed(false),
      connected(false),
      q_ptr(q)
{
}

void ConnectionPrivate::_q_heartbeat()
{
    Frame::Heartbeat frame;
    network->sendFrame(frame);
}

}   // namespace QAMQP

//////////////////////////////////////////////////////////////////////////

using namespace QAMQP;
Connection::Connection(Network *network, Client *client)
    : QObject(client),
      d_ptr(new ConnectionPrivate(this))
{
    Q_D(Connection);
    d->client = client;
    d->network = network;
    d->heartbeatTimer = new QTimer(this);
    connect(d->heartbeatTimer, SIGNAL(timeout()), this, SLOT(_q_heartbeat()));
}

Connection::~Connection()
{
}

void Connection::start(const Frame::Method &frame)
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

void Connection::secure(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    qDebug() << Q_FUNC_INFO << "called!";
}

void Connection::tune(const Frame::Method &frame)
{
    Q_D(Connection);
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

    if (d->heartbeatTimer) {
        d->heartbeatTimer->setInterval(heartbeat * 1000);
        if (d->heartbeatTimer->interval())
            d->heartbeatTimer->start();
        else
            d->heartbeatTimer->stop();
    }

    tuneOk();
    open();
}

void Connection::openOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_D(Connection);
    qDebug(">> OpenOK");
    d->connected = true;
    Q_EMIT connected();
}

void Connection::closeOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_D(Connection);

    qDebug() << Q_FUNC_INFO << "received";

    d->connected = false;
    if (d->heartbeatTimer)
        d->heartbeatTimer->stop();
    Q_EMIT disconnected();
}

void Connection::close(const Frame::Method &frame)
{
    Q_D(Connection);
    qDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code_ = 0, classId, methodId;
    stream >> code_;
    QString text(Frame::readField('s', stream).toString());
    stream >> classId;
    stream >> methodId;

    qDebug(">> code: %d", code_);
    qDebug(">> text: %s", qPrintable(text));
    qDebug(">> class-id: %d", classId);
    qDebug(">> method-id: %d", methodId);
    d->connected = false;
    d->network->error(QAbstractSocket::RemoteHostClosedError);
    Q_EMIT disconnected();
}

void Connection::startOk()
{
    Q_D(Connection);
    Frame::Method frame(Frame::fcConnection, ConnectionPrivate::miStartOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::TableField clientProperties;
    clientProperties["version"] = QString(QAMQP_VERSION);
    clientProperties["platform"] = QString("Qt %1").arg(qVersion());
    clientProperties["product"] = QString("QAMQP");
    clientProperties.unite(d->customProperty);
    Frame::serialize(stream, clientProperties);

    d->client->auth()->write(stream);
    Frame::writeField('s', stream, "en_US");

    frame.setArguments(arguments);
    d->network->sendFrame(frame);
}

void Connection::secureOk()
{
    qDebug() << Q_FUNC_INFO;
}

void Connection::tuneOk()
{
    Q_D(Connection);
    Frame::Method frame(Frame::fcConnection, ConnectionPrivate::miTuneOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0); //channel_max
    stream << qint32(FRAME_MAX); //frame_max
    stream << qint16(d->heartbeatTimer->interval() / 1000); //heartbeat

    frame.setArguments(arguments);
    d->network->sendFrame(frame);
}

void Connection::open()
{
    Q_D(Connection);
    Frame::Method frame(Frame::fcConnection, ConnectionPrivate::miOpen);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, d->client->virtualHost());

    stream << qint8(0);
    stream << qint8(0);

    frame.setArguments(arguments);
    d->network->sendFrame(frame);
}

void Connection::close(int code, const QString &text, int classId, int methodId)
{
    Q_D(Connection);
    Frame::Method frame(Frame::fcConnection, ConnectionPrivate::miClose);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, d->client->virtualHost());

    stream << qint16(code);
    Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments);
    d->network->sendFrame(frame);
}

void Connection::closeOk()
{
    Q_D(Connection);
    Frame::Method frame(Frame::fcConnection, ConnectionPrivate::miCloseOk);
    d->connected = false;
    d->network->sendFrame(frame);
}

void Connection::_q_method(const Frame::Method &frame)
{
    Q_D(Connection);
    Q_ASSERT(frame.methodClass() == Frame::fcConnection);
    if (frame.methodClass() != Frame::fcConnection)
        return;

    qDebug() << "Connection:";

    if (d->closed) {
        if (frame.id() == ConnectionPrivate::miCloseOk)
            closeOk(frame);
        return;
    }

    switch (ConnectionPrivate::MethodId(frame.id())) {
    case ConnectionPrivate::miStart:
        start(frame);
        break;
    case ConnectionPrivate::miSecure:
        secure(frame);
        break;
    case ConnectionPrivate::miTune:
        tune(frame);
        break;
    case ConnectionPrivate::miOpenOk:
        openOk(frame);
        break;
    case ConnectionPrivate::miClose:
        close(frame);
        break;
    case ConnectionPrivate::miCloseOk:
        closeOk(frame);
        break;
    default:
        qWarning("Unknown method-id %d", frame.id());
    }
}

bool Connection::isConnected() const
{
    Q_D(const Connection);
    return d->connected;
}

void Connection::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_D(Connection);

    // NOTE: these were hardcoded values, could be bad
    int channel = 0;
    bool global = true;

    Frame::Method frame(Frame::fcBasic, 10);
    frame.setChannel(channel);
    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << prefetchSize;
    out << prefetchCount;
    out << qint8(global ? 1 : 0);

    frame.setArguments(arguments);
    d->network->sendFrame(frame);
}

void Connection::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(Connection);
    d->customProperty[name] = value;
}

QString Connection::customProperty(const QString &name) const
{
    Q_D(const Connection);
    if (d->customProperty.contains(name))
        return d->customProperty.value(name).toString();
    return QString();
}

#include "moc_amqp_connection_p.cpp"
