#include "amqp_connection.h"
#include "amqp_connection_p.h"
#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_frame.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QVariant>
#include <QTimer>

using namespace QAMQP;

ConnectionPrivate::ConnectionPrivate(Connection * q)
    : closed_(false),
      connected(false),
      q_ptr(q)
{
}

ConnectionPrivate::~ConnectionPrivate()
{
}

void ConnectionPrivate::init(Client * parent)
{
    Q_Q(QAMQP::Connection);
    q->setParent(parent);
    client_ = parent;
    heartbeatTimer_ = new QTimer(parent);
    QObject::connect(heartbeatTimer_, SIGNAL(timeout()), q, SLOT(_q_heartbeat()));
}

void ConnectionPrivate::startOk()
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miStartOk);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    QAMQP::Frame::TableField clientProperties;
    clientProperties["version"] = QString(QAMQP_VERSION);
    clientProperties["platform"] = QString("Qt %1").arg(qVersion());
    clientProperties["product"] = QString("QAMQP");
    clientProperties.unite(customProperty);
    QAMQP::Frame::serialize(stream, clientProperties);

    client_->d_func()->auth_->write(stream);
    QAMQP::Frame::writeField('s', stream, "en_US");
    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::secureOk()
{
}

void ConnectionPrivate::tuneOk()
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miTuneOk);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    stream << qint16(0); //channel_max
    stream << qint32(FRAME_MAX); //frame_max
    stream << qint16(heartbeatTimer_->interval() / 1000); //heartbeat

    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::open()
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miOpen);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    QAMQP::Frame::writeField('s',stream, client_->virtualHost());

    stream << qint8(0);
    stream << qint8(0);

    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::start(const QAMQP::Frame::Method &frame)
{
    qDebug(">> Start");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    quint8 version_major = 0;
    quint8 version_minor = 0;

    stream >> version_major >> version_minor;

    QAMQP::Frame::TableField table;
    QAMQP::Frame::deserialize(stream, table);

    QString mechanisms = QAMQP::Frame::readField('S', stream).toString();
    QString locales = QAMQP::Frame::readField('S', stream).toString();

    qDebug(">> version_major: %d", version_major);
    qDebug(">> version_minor: %d", version_minor);

    QAMQP::Frame::print(table);

    qDebug(">> mechanisms: %s", qPrintable(mechanisms));
    qDebug(">> locales: %s", qPrintable(locales));

    startOk();
}

void ConnectionPrivate::secure(const QAMQP::Frame::Method &frame)
{
    Q_UNUSED(frame)
}

void ConnectionPrivate::tune(const QAMQP::Frame::Method &frame)
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

    if (heartbeatTimer_) {
        heartbeatTimer_->setInterval(heartbeat * 1000);
        if (heartbeatTimer_->interval())
            heartbeatTimer_->start();
        else
            heartbeatTimer_->stop();
    }

    tuneOk();
    open();
}

void ConnectionPrivate::openOk(const QAMQP::Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAMQP::Connection);

    qDebug(">> OpenOK");
    connected = true;
    q->openOk();
}

void ConnectionPrivate::close(const QAMQP::Frame::Method &frame)
{
    Q_Q(QAMQP::Connection);

    qDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code_ = 0, classId, methodId;
    stream >> code_;
    QString text(QAMQP::Frame::readField('s', stream).toString());
    stream >> classId;
    stream >> methodId;

    qDebug(">> code: %d", code_);
    qDebug(">> text: %s", qPrintable(text));
    qDebug(">> class-id: %d", classId);
    qDebug(">> method-id: %d", methodId);
    connected = false;
    client_->d_func()->network_->error(QAbstractSocket::RemoteHostClosedError);
    QMetaObject::invokeMethod(q, "disconnected");
}

void ConnectionPrivate::close(int code, const QString &text, int classId, int methodId)
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miClose);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    QAMQP::Frame::writeField('s',stream, client_->virtualHost());

    stream << qint16(code);
    QAMQP::Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::closeOk()
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miCloseOk);
    connected = false;
    client_->d_func()->network_->sendFrame(frame);

}

void ConnectionPrivate::closeOk(const QAMQP::Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(QAMQP::Connection);

    connected = false;
    QMetaObject::invokeMethod(q, "disconnected");
    if (heartbeatTimer_)
        heartbeatTimer_->stop();
}


void ConnectionPrivate::setQOS(qint32 prefetchSize, quint16 prefetchCount, int channel, bool global)
{
    QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, 10);
    frame.setChannel(channel);
    QByteArray arguments_;
    QDataStream out(&arguments_, QIODevice::WriteOnly);

    out << prefetchSize;
    out << prefetchCount;
    out << qint8(global ? 1 : 0);

    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

bool ConnectionPrivate::_q_method(const QAMQP::Frame::Method &frame)
{
    Q_ASSERT(frame.methodClass() == QAMQP::Frame::fcConnection);
    if (frame.methodClass() != QAMQP::Frame::fcConnection)
        return true;

    qDebug() << "Connection:";

    if (closed_) {
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
    QAMQP::Frame::Heartbeat frame;
    client_->d_func()->network_->sendFrame(frame);
}

//////////////////////////////////////////////////////////////////////////

Connection::Connection(Client *parent)
    : QObject(parent),
      d_ptr(new ConnectionPrivate(this))
{
    Q_D(QAMQP::Connection);
    d->init(parent);
}

Connection::~Connection()
{
}

void Connection::startOk()
{
    Q_D(QAMQP::Connection);
    d->startOk();
}

void Connection::secureOk()
{
    Q_D(QAMQP::Connection);
    d->secureOk();
}

void Connection::tuneOk()
{
    Q_D(QAMQP::Connection);
    d->tuneOk();
}

void Connection::open()
{
    Q_D(QAMQP::Connection);
    d->open();
}

void Connection::close(int code, const QString &text, int classId , int methodId)
{
    Q_D(QAMQP::Connection);
    d->close(code, text, classId, methodId);
}

void Connection::closeOk()
{
    Q_D(QAMQP::Connection);
    d->closeOk();
    Q_EMIT disconnect();
}

void Connection::openOk()
{
    Q_EMIT connected();
}

void Connection::_q_method(const QAMQP::Frame::Method &frame)
{
    Q_D(QAMQP::Connection);
    d->_q_method(frame);
}

bool Connection::isConnected() const
{
    Q_D(const QAMQP::Connection);
    return d->connected;
}

void Connection::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_D(QAMQP::Connection);
    d->setQOS(prefetchSize, prefetchCount, 0, true);
}

void Connection::addCustomProperty(const QString &name, const QString &value)
{
    Q_D(QAMQP::Connection);
    d->customProperty[name] = value;
}

QString Connection::customProperty(const QString &name) const
{
    Q_D(const QAMQP::Connection);
    if (d->customProperty.contains(name))
        return d->customProperty.value(name).toString();
    return QString();
}

#include "moc_amqp_connection.cpp"
