#include "amqp_connection.h"
#include "amqp_connection_p.h"
#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_frame.h"
#include "amqp_global.h"

#include <QDebug>
#include <QDataStream>
#include <QTimer>

using namespace QAMQP;

ConnectionPrivate::ConnectionPrivate(Connection *q)
    : closed(false),
      connected(false),
      q_ptr(q)
{
}

ConnectionPrivate::~ConnectionPrivate()
{
}

void ConnectionPrivate::startOk()
{
    Frame::Method frame(Frame::fcConnection, miStartOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::TableField clientProperties;
    clientProperties["version"] = QString(QAMQP_VERSION);
    clientProperties["platform"] = QString("Qt %1").arg(qVersion());
    clientProperties["product"] = QString("QAMQP");
    clientProperties.unite(customProperty);
    Frame::serialize(stream, clientProperties);

    client->auth()->write(stream);
    Frame::writeField('s', stream, "en_US");

    frame.setArguments(arguments);
    network->sendFrame(frame);
}

void ConnectionPrivate::secureOk()
{
}

void ConnectionPrivate::tuneOk()
{
    Frame::Method frame(Frame::fcConnection, miTuneOk);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    stream << qint16(0); //channel_max
    stream << qint32(FRAME_MAX); //frame_max
    stream << qint16(heartbeatTimer->interval() / 1000); //heartbeat

    frame.setArguments(arguments);
    network->sendFrame(frame);
}

void ConnectionPrivate::open()
{
    Frame::Method frame(Frame::fcConnection, miOpen);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, client->virtualHost());

    stream << qint8(0);
    stream << qint8(0);

    frame.setArguments(arguments);
    network->sendFrame(frame);
}

void ConnectionPrivate::start(const Frame::Method &frame)
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

void ConnectionPrivate::secure(const Frame::Method &frame)
{
    Q_UNUSED(frame)
}

void ConnectionPrivate::tune(const Frame::Method &frame)
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

void ConnectionPrivate::openOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Connection);

    qDebug(">> OpenOK");
    connected = true;
    q->openOk();
}

void ConnectionPrivate::close(const Frame::Method &frame)
{
    Q_Q(Connection);

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
    connected = false;
    network->error(QAbstractSocket::RemoteHostClosedError);
    Q_EMIT q->disconnected();
}

void ConnectionPrivate::close(int code, const QString &text, int classId, int methodId)
{
    Frame::Method frame(Frame::fcConnection, miClose);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, client->virtualHost());

    stream << qint16(code);
    Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments);
    network->sendFrame(frame);
}

void ConnectionPrivate::closeOk()
{
    Frame::Method frame(Frame::fcConnection, miCloseOk);
    connected = false;
    network->sendFrame(frame);
}

void ConnectionPrivate::closeOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Connection);

    connected = false;
    Q_EMIT q->disconnected();
    if (heartbeatTimer)
        heartbeatTimer->stop();
}

void ConnectionPrivate::setQOS(qint32 prefetchSize, quint16 prefetchCount, int channel, bool global)
{
    Frame::Method frame(Frame::fcBasic, 10);
    frame.setChannel(channel);
    QByteArray arguments;
    QDataStream out(&arguments, QIODevice::WriteOnly);

    out << prefetchSize;
    out << prefetchCount;
    out << qint8(global ? 1 : 0);

    frame.setArguments(arguments);
    network->sendFrame(frame);
}

bool ConnectionPrivate::_q_method(const Frame::Method &frame)
{
    Q_ASSERT(frame.methodClass() == Frame::fcConnection);
    if (frame.methodClass() != Frame::fcConnection)
        return true;

    qDebug() << "Connection:";

    if (closed) {
        if (frame.id() == miCloseOk)
            closeOk(frame);

        return true;
    }

    switch (MethodId(frame.id())) {
    case miStart:
        start(frame);
        break;
    case miSecure:
        secure(frame);
        break;
    case miTune:
        tune(frame);
        break;
    case miOpenOk:
        openOk(frame);
        break;
    case miClose:
        close(frame);
        break;
    case miCloseOk:
        closeOk(frame);
        break;
    default:
        qWarning("Unknown method-id %d", frame.id());
        return false;
    }

    return true;
}

void ConnectionPrivate::_q_heartbeat()
{
    Frame::Heartbeat frame;
    network->sendFrame(frame);
}

//////////////////////////////////////////////////////////////////////////

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

void Connection::startOk()
{
    Q_D(Connection);
    d->startOk();
}

void Connection::secureOk()
{
    Q_D(Connection);
    d->secureOk();
}

void Connection::tuneOk()
{
    Q_D(Connection);
    d->tuneOk();
}

void Connection::open()
{
    Q_D(Connection);
    d->open();
}

void Connection::close(int code, const QString &text, int classId , int methodId)
{
    Q_D(Connection);
    d->close(code, text, classId, methodId);
}

void Connection::closeOk()
{
    Q_D(Connection);
    d->closeOk();
    Q_EMIT disconnected();
}

void Connection::openOk()
{
    Q_EMIT connected();
}

void Connection::_q_method(const Frame::Method &frame)
{
    Q_D(Connection);
    d->_q_method(frame);
}

bool Connection::isConnected() const
{
    Q_D(const Connection);
    return d->connected;
}

void Connection::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_D(Connection);
    d->setQOS(prefetchSize, prefetchCount, 0, true);
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

#include "moc_amqp_connection.cpp"
