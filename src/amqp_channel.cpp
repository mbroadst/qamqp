#include "amqp_channel.h"
#include "amqp_channel_p.h"
#include "amqp_client.h"
#include "amqp_client_p.h"
#include "amqp_connection_p.h"

#include <QDebug>
#include <QDataStream>

using namespace QAMQP;

//////////////////////////////////////////////////////////////////////////

Channel::Channel(int channelNumber, Client *parent)
    : QObject(parent),
      d_ptr(new ChannelPrivate(this))
{
    Q_D(Channel);
    d->init(channelNumber, parent);
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
    return d->number;
}

void Channel::setName(const QString &name)
{
    Q_D(Channel);
    d->name = name;
}

void Channel::stateChanged(int state)
{
    switch(ChannelPrivate::State(state)) {
    case ChannelPrivate::csOpened:
        emit opened();
        break;
    case ChannelPrivate::csClosed:
        emit closed();
        break;
    case ChannelPrivate::csIdle:
        emit flowChanged(false);
        break;
    case ChannelPrivate::csRunning:
        emit flowChanged(true);
        break;
    }
}

void Channel::_q_method(const Frame::Method &frame)
{
    Q_D(Channel);
    d->_q_method(frame);
}

bool Channel::isOpened() const
{
    Q_D(const Channel);
    return d->opened;
}

void Channel::onOpen()
{
}

void Channel::onClose()
{
}

void Channel::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_D(Channel);
    d->setQOS(prefetchSize, prefetchCount);
}

//////////////////////////////////////////////////////////////////////////

int ChannelPrivate::nextChannelNumber_ = 0;
ChannelPrivate::ChannelPrivate(Channel *q)
    : number(0),
      opened(false),
      needOpen(true),
      q_ptr(q)
{
}

ChannelPrivate::~ChannelPrivate()
{
}

void ChannelPrivate::init(int channelNumber, Client *parent)
{
    Q_Q(Channel);
    needOpen = channelNumber == -1 ? true : false;
    number = channelNumber == -1 ? ++nextChannelNumber_ : channelNumber;
    nextChannelNumber_ = qMax(channelNumber, (nextChannelNumber_ + 1));
    q->setParent(parent);
    client_ = parent;
}

bool ChannelPrivate::_q_method(const Frame::Method &frame)
{
    Q_ASSERT(frame.channel() == number);
    if (frame.channel() != number)
        return true;

    if (frame.methodClass() != Frame::fcChannel)
        return false;

    qDebug("Channel#%d:", number);

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
    if (client_)
        client_->d_func()->network_->sendFrame(frame);
}

void ChannelPrivate::open()
{
    if (!needOpen || opened)
        return;

    if (!client_->d_func()->connection_->isConnected())
        return;

    qDebug("Open channel #%d", number);
    Frame::Method frame(Frame::fcChannel, miOpen);
    frame.setChannel(number);
    QByteArray arguments_;
    arguments_.resize(1);
    arguments_[0] = 0;
    frame.setArguments(arguments_);
    sendFrame(frame);
}

void ChannelPrivate::flow()
{
}

void ChannelPrivate::flow(const Frame::Method &frame)
{
    Q_UNUSED(frame);
}

void ChannelPrivate::flowOk()
{
}

void ChannelPrivate::flowOk(const Frame::Method &frame)
{
    Q_UNUSED(frame);
}

void ChannelPrivate::close(int code, const QString &text, int classId, int methodId)
{
    Frame::Method frame(Frame::fcChannel, miClose);
    QByteArray arguments_;
    QDataStream stream(&arguments_, QIODevice::WriteOnly);

    Frame::writeField('s',stream, client_->virtualHost());

    stream << qint16(code);
    Frame::writeField('s', stream, text);
    stream << qint16(classId);
    stream << qint16(methodId);

    frame.setArguments(arguments_);
    client_->d_func()->network_->sendFrame(frame);
}

void ChannelPrivate::close(const Frame::Method &frame)
{
    Q_Q(Channel);
    q->stateChanged(csClosed);

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

    q->stateChanged(csClosed);
    q->onClose();
    opened = false;
}

void ChannelPrivate::openOk(const Frame::Method &frame)
{
    Q_UNUSED(frame)
    Q_Q(Channel);

    qDebug(">> OpenOK");
    opened = true;
    q->stateChanged(csOpened);
    q->onOpen();
}

void ChannelPrivate::setQOS(qint32 prefetchSize, quint16 prefetchCount)
{
    Q_UNUSED(prefetchSize)
    Q_UNUSED(prefetchCount)
    qDebug() << Q_FUNC_INFO << "temporarily disabled";
//    client_->d_func()->connection_->d_func()->setQOS(prefetchSize, prefetchCount, number, false);
}

void ChannelPrivate::_q_disconnected()
{
    nextChannelNumber_ = 0;
    opened = false;
}

#include "moc_amqp_channel.cpp"
