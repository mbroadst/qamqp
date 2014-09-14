#include "qamqpchannel.h"
#include "qamqpchannel_p.h"
#include "qamqpclient.h"
#include "qamqpclient_p.h"

#include <QDebug>
#include <QDataStream>

namespace QAMQP {

int ChannelPrivate::nextChannelNumber = 0;
ChannelPrivate::ChannelPrivate(Channel *q)
    : channelNumber(0),
      opened(false),
      needOpen(true),
      prefetchSize(0),
      requestedPrefetchSize(0),
      prefetchCount(0),
      requestedPrefetchCount(0),
      error(QAMQP::NoError),
      q_ptr(q)
{
}

ChannelPrivate::~ChannelPrivate()
{
}

void ChannelPrivate::init(int channel, Client *c)
{
    client = c;
    needOpen = channel == -1 ? true : false;
    channelNumber = channel == -1 ? ++nextChannelNumber : channel;
    nextChannelNumber = qMax(channelNumber, (nextChannelNumber + 1));
}

bool ChannelPrivate::_q_method(const Frame::Method &frame)
{
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return true;

    if (frame.methodClass() == Frame::fcBasic) {
        if (frame.id() == bmQosOk) {
            qosOk(frame);
            return true;
        }

        return false;
    }

    if (frame.methodClass() != Frame::fcChannel)
        return false;

    qAmqpDebug("Channel#%d:", channelNumber);

    switch (frame.id()) {
    case miOpenOk:
        openOk(frame);
        break;
    case miFlow:
        flow(frame);
        break;
    case miFlowOk:
        flowOk(frame);
        break;
    case miClose:
        close(frame);
        break;
    case miCloseOk:
        closeOk(frame);
        break;
    }

    return true;
}

void ChannelPrivate::_q_open()
{
    open();
}

void ChannelPrivate::sendFrame(const Frame::Base &frame)
{
    if (!client) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid client";
        return;
    }

    client->d_func()->sendFrame(frame);
}

void ChannelPrivate::open()
{
    if (!needOpen || opened)
        return;

    if (!client->isConnected())
        return;

    qAmqpDebug("Open channel #%d", channelNumber);
    Frame::Method frame(Frame::fcChannel, miOpen);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    arguments.resize(1);
    arguments[0] = 0;

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ChannelPrivate::flow(bool active)
{
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);
    Frame::writeAmqpField(stream, MetaType::ShortShortUint, (active ? 1 : 0));

    Frame::Method frame(Frame::fcChannel, miFlow);
    frame.setChannel(channelNumber);
    frame.setArguments(arguments);
    sendFrame(frame);
}

// NOTE: not implemented until I can figure out a good way to force the server
//       to pause the channel in a test. It seems like RabbitMQ just doesn't
//       care about flow control, preferring rather to use basic.qos
void ChannelPrivate::flow(const Frame::Method &frame)
{
    Q_UNUSED(frame);
    qAmqpDebug() << Q_FUNC_INFO;
}

void ChannelPrivate::flowOk()
{
    qAmqpDebug() << Q_FUNC_INFO;
}

void ChannelPrivate::flowOk(const Frame::Method &frame)
{
    Q_Q(Channel);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    bool active = Frame::readAmqpField(stream, MetaType::Boolean).toBool();
    if (active)
        Q_EMIT q->resumed();
    else
        Q_EMIT q->paused();
}

void ChannelPrivate::close(int code, const QString &text, int classId, int methodId)
{
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    if (!code) code = 200;
    Frame::writeAmqpField(stream, MetaType::ShortUint, code);
    if (!text.isEmpty()) {
      Frame::writeAmqpField(stream, MetaType::ShortString, text);
    } else {
      Frame::writeAmqpField(stream, MetaType::ShortString, QLatin1String("OK"));
    }

    Frame::writeAmqpField(stream, MetaType::ShortUint, classId);
    Frame::writeAmqpField(stream, MetaType::ShortUint, methodId);

    Frame::Method frame(Frame::fcChannel, miClose);
    frame.setChannel(channelNumber);
    frame.setArguments(arguments);
    sendFrame(frame);
}

void ChannelPrivate::close(const Frame::Method &frame)
{
    Q_Q(Channel);
    qAmqpDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text = Frame::readAmqpField(stream, MetaType::ShortString).toString();

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
    Q_EMIT q->closed();

    // complete handshake
    Frame::Method closeOkFrame(Frame::fcChannel, miCloseOk);
    closeOkFrame.setChannel(channelNumber);
    sendFrame(closeOkFrame);
}

void ChannelPrivate::closeOk(const Frame::Method &)
{
    Q_Q(Channel);
    Q_EMIT q->closed();
    q->channelClosed();
    opened = false;
}

void ChannelPrivate::openOk(const Frame::Method &)
{
    Q_Q(Channel);
    qAmqpDebug(">> OpenOK");
    opened = true;
    Q_EMIT q->opened();
    q->channelOpened();
}

void ChannelPrivate::_q_disconnected()
{
    nextChannelNumber = 0;
    opened = false;
}

void ChannelPrivate::qosOk(const Frame::Method &frame)
{
    Q_Q(Channel);
    Q_UNUSED(frame)

    prefetchCount = requestedPrefetchCount;
    prefetchSize = requestedPrefetchSize;
    Q_EMIT q->qosDefined();
}

//////////////////////////////////////////////////////////////////////////

Channel::Channel(ChannelPrivate *dd, Client *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

Channel::~Channel()
{
}

void Channel::close()
{
    Q_D(Channel);
    d->needOpen = true;
    if (d->opened)
        d->close(0, QString(), 0,0);
}

void Channel::reopen()
{
    Q_D(Channel);
    if (d->opened)
        close();
    d->open();
}

QString Channel::name() const
{
    Q_D(const Channel);
    return d->name;
}

int Channel::channelNumber() const
{
    Q_D(const Channel);
    return d->channelNumber;
}

void Channel::setName(const QString &name)
{
    Q_D(Channel);
    d->name = name;
}

bool Channel::isOpened() const
{
    Q_D(const Channel);
    return d->opened;
}

void Channel::qos(qint16 prefetchCount, qint32 prefetchSize)
{
    Q_D(Channel);
    Frame::Method frame(Frame::fcBasic, ChannelPrivate::bmQos);
    frame.setChannel(d->channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    d->requestedPrefetchSize = prefetchSize;
    d->requestedPrefetchCount = prefetchCount;

    stream << qint32(prefetchSize);
    stream << qint16(prefetchCount);
    stream << qint8(0x0);   // global

    frame.setArguments(arguments);
    d->sendFrame(frame);
}

qint32 Channel::prefetchSize() const
{
    Q_D(const Channel);
    return d->prefetchSize;
}

qint16 Channel::prefetchCount() const
{
    Q_D(const Channel);
    return d->prefetchCount;
}

Error Channel::error() const
{
    Q_D(const Channel);
    return d->error;
}

QString Channel::errorString() const
{
    Q_D(const Channel);
    return d->errorString;
}

void Channel::resume()
{
    Q_D(Channel);
    d->flow(true);
}

} // namespace QAMQP

#include "moc_qamqpchannel.cpp"
