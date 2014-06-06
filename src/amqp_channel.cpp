#include "amqp_channel.h"
#include "amqp_channel_p.h"
#include "amqp_client.h"
#include "amqp_client_p.h"

#include <QDebug>
#include <QDataStream>

using namespace QAMQP;

int ChannelPrivate::nextChannelNumber = 0;
ChannelPrivate::ChannelPrivate(Channel *q)
    : channelNumber(0),
      opened(false),
      needOpen(true),
      error(Channel::NoError),
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

void ChannelPrivate::stateChanged(State state)
{
    Q_Q(Channel);
    switch(ChannelPrivate::State(state)) {
    case ChannelPrivate::csOpened:
        Q_EMIT q->opened();
        break;
    case ChannelPrivate::csClosed:
        Q_EMIT q->closed();
        break;
    case ChannelPrivate::csIdle:
        Q_EMIT q->flowChanged(false);
        break;
    case ChannelPrivate::csRunning:
        Q_EMIT q->flowChanged(true);
        break;
    }
}

bool ChannelPrivate::_q_method(const Frame::Method &frame)
{
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return true;

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
    if (client)
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

void ChannelPrivate::flow()
{
}

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
    Q_UNUSED(frame);
    qAmqpDebug() << Q_FUNC_INFO;
}

void ChannelPrivate::close(int code, const QString &text, int classId, int methodId)
{
    Frame::Method frame(Frame::fcChannel, miClose);
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    Frame::writeField('s',stream, client->virtualHost());

    stream << qint16(code);
    Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments);
    sendFrame(frame);
}

void ChannelPrivate::close(const Frame::Method &frame)
{
    Q_Q(Channel);
    qAmqpDebug(">> CLOSE");
    stateChanged(csClosed);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text(Frame::readField('s', stream).toString());
    stream >> classId;
    stream >> methodId;

    Channel::ChannelError checkError = static_cast<Channel::ChannelError>(code);
    if (checkError != Channel::NoError) {
        error = checkError;
        errorString = qPrintable(text);
        Q_EMIT q->error(error);
    }

    qAmqpDebug(">> code: %d", code);
    qAmqpDebug(">> text: %s", qPrintable(text));
    qAmqpDebug(">> class-id: %d", classId);
    qAmqpDebug(">> method-id: %d", methodId);
}

void ChannelPrivate::closeOk()
{
    Frame::Method frame(Frame::fcChannel, miCloseOk);
    sendFrame(frame);
}

void ChannelPrivate::closeOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Channel);

    stateChanged(csClosed);
    q->channelClosed();
    opened = false;
}

void ChannelPrivate::openOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Channel);

    qAmqpDebug(">> OpenOK");
    opened = true;
    stateChanged(csOpened);
    q->channelOpened();
}

void ChannelPrivate::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_UNUSED(prefetchSize)
    Q_UNUSED(prefetchCount)
    qAmqpDebug() << Q_FUNC_INFO << "temporarily disabled";
//    client_->d_func()->connection_->d_func()->setQOS(prefetchSize, prefetchCount, channelNumber, false);
}

void ChannelPrivate::_q_disconnected()
{
    nextChannelNumber = 0;
    opened = false;
}

//////////////////////////////////////////////////////////////////////////

Channel::Channel(int channelNumber, Client *client)
    : QObject(client),
      d_ptr(new ChannelPrivate(this))
{
    Q_D(Channel);
    d->init(channelNumber, client);
}

Channel::Channel(ChannelPrivate *dd, Client *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

Channel::~Channel()
{
}

void Channel::closeChannel()
{
    Q_D(Channel);
    d->needOpen = true;
    if (d->opened)
        d->close(0, QString(), 0,0);
}

void Channel::reopen()
{
    Q_D(Channel);
    closeChannel();
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

void Channel::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_D(Channel);
    d->setQOS(prefetchSize, prefetchCount);
}

Channel::ChannelError Channel::error() const
{
    Q_D(const Channel);
    return d->error;
}

QString Channel::errorString() const
{
    Q_D(const Channel);
    return d->errorString;
}

#include "moc_amqp_channel.cpp"
