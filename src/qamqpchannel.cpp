#include <QDataStream>
#include <QDebug>

#include "qamqpchannel.h"
#include "qamqpchannel_p.h"
#include "qamqpclient.h"
#include "qamqpclient_p.h"

quint16 QAmqpChannelPrivate::nextChannelNumber = 0;
QAmqpChannelPrivate::QAmqpChannelPrivate(QAmqpChannel *q)
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

QAmqpChannelPrivate::~QAmqpChannelPrivate()
{
    if (!client.isNull()) {
        QAmqpClientPrivate *priv = client->d_func();
        priv->methodHandlersByChannel[channelNumber].removeAll(this);
    }
}

void QAmqpChannelPrivate::init(int channel, QAmqpClient *c)
{
    client = c;
    needOpen = channel == -1 ? true : false;
    channelNumber = channel == -1 ? ++nextChannelNumber : channel;
    nextChannelNumber = qMax(channelNumber, nextChannelNumber);
}

bool QAmqpChannelPrivate::_q_method(const QAmqpMethodFrame &frame)
{
    Q_ASSERT(frame.channel() == channelNumber);
    if (frame.channel() != channelNumber)
        return true;

    if (frame.methodClass() == QAmqpFrame::Basic) {
        if (frame.id() == bmQosOk) {
            qosOk(frame);
            return true;
        }

        return false;
    }

    if (frame.methodClass() != QAmqpFrame::Channel)
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

void QAmqpChannelPrivate::_q_open()
{
    open();
}

void QAmqpChannelPrivate::sendFrame(const QAmqpFrame &frame)
{
    if (!client) {
        qAmqpDebug() << Q_FUNC_INFO << "invalid client";
        return;
    }

    client->d_func()->sendFrame(frame);
}

void QAmqpChannelPrivate::open()
{
    if (!needOpen || opened)
        return;

    if (!client->isConnected())
        return;

    qAmqpDebug("Open channel #%d", channelNumber);
    QAmqpMethodFrame frame(QAmqpFrame::Channel, miOpen);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    arguments.resize(1);
    arguments[0] = 0;

    frame.setArguments(arguments);
    sendFrame(frame);
}

void QAmqpChannelPrivate::flow(bool active)
{
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortShortUint, (active ? 1 : 0));

    QAmqpMethodFrame frame(QAmqpFrame::Channel, miFlow);
    frame.setChannel(channelNumber);
    frame.setArguments(arguments);
    sendFrame(frame);
}

// NOTE: not implemented until I can figure out a good way to force the server
//       to pause the channel in a test. It seems like RabbitMQ just doesn't
//       care about flow control, preferring rather to use basic.qos
void QAmqpChannelPrivate::flow(const QAmqpMethodFrame &frame)
{
    Q_UNUSED(frame);
    qAmqpDebug() << Q_FUNC_INFO;
}

void QAmqpChannelPrivate::flowOk()
{
    qAmqpDebug() << Q_FUNC_INFO;
}

void QAmqpChannelPrivate::flowOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpChannel);
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    bool active = QAmqpFrame::readAmqpField(stream, QAmqpMetaType::Boolean).toBool();
    if (active)
        Q_EMIT q->resumed();
    else
        Q_EMIT q->paused();
}

void QAmqpChannelPrivate::close(int code, const QString &text, int classId, int methodId)
{
    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);

    if (!code) code = 200;
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortUint, code);
    if (!text.isEmpty()) {
      QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, text);
    } else {
      QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortString, QLatin1String("OK"));
    }

    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortUint, classId);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortUint, methodId);

    QAmqpMethodFrame frame(QAmqpFrame::Channel, miClose);
    frame.setChannel(channelNumber);
    frame.setArguments(arguments);
    sendFrame(frame);
}

void QAmqpChannelPrivate::close(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpChannel);
    qAmqpDebug(">> CLOSE");
    QByteArray data = frame.arguments();
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint16 code = 0, classId, methodId;
    stream >> code;
    QString text =
        QAmqpFrame::readAmqpField(stream, QAmqpMetaType::ShortString).toString();

    stream >> classId;
    stream >> methodId;

    QAMQP::Error checkError = static_cast<QAMQP::Error>(code);
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
    QAmqpMethodFrame closeOkFrame(QAmqpFrame::Channel, miCloseOk);
    closeOkFrame.setChannel(channelNumber);
    sendFrame(closeOkFrame);
}

void QAmqpChannelPrivate::closeOk(const QAmqpMethodFrame &)
{
    Q_Q(QAmqpChannel);
    Q_EMIT q->closed();
    q->channelClosed();
    opened = false;
}

void QAmqpChannelPrivate::openOk(const QAmqpMethodFrame &)
{
    Q_Q(QAmqpChannel);
    qAmqpDebug(">> OpenOK");
    opened = true;
    Q_EMIT q->opened();
    q->channelOpened();
}

void QAmqpChannelPrivate::_q_disconnected()
{
    nextChannelNumber = 0;
    opened = false;
}

void QAmqpChannelPrivate::qosOk(const QAmqpMethodFrame &frame)
{
    Q_Q(QAmqpChannel);
    Q_UNUSED(frame)

    prefetchCount = requestedPrefetchCount;
    prefetchSize = requestedPrefetchSize;
    Q_EMIT q->qosDefined();
}

//////////////////////////////////////////////////////////////////////////

QAmqpChannel::QAmqpChannel(QAmqpChannelPrivate *dd, QAmqpClient *parent)
    : QObject(parent),
      d_ptr(dd)
{
}

QAmqpChannel::~QAmqpChannel()
{
}

void QAmqpChannel::close()
{
    Q_D(QAmqpChannel);
    d->needOpen = true;
    if (d->opened)
        d->close(0, QString(), 0,0);
}

void QAmqpChannel::reopen()
{
    Q_D(QAmqpChannel);
    if (d->opened)
        close();
    d->open();
}

QString QAmqpChannel::name() const
{
    Q_D(const QAmqpChannel);
    return d->name;
}

int QAmqpChannel::channelNumber() const
{
    Q_D(const QAmqpChannel);
    return d->channelNumber;
}

void QAmqpChannel::setName(const QString &name)
{
    Q_D(QAmqpChannel);
    d->name = name;
}

bool QAmqpChannel::isOpen() const
{
    Q_D(const QAmqpChannel);
    return d->opened;
}

void QAmqpChannel::qos(qint16 prefetchCount, qint32 prefetchSize)
{
    Q_D(QAmqpChannel);
    QAmqpMethodFrame frame(QAmqpFrame::Basic, QAmqpChannelPrivate::bmQos);
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

qint32 QAmqpChannel::prefetchSize() const
{
    Q_D(const QAmqpChannel);
    return d->prefetchSize;
}

qint16 QAmqpChannel::prefetchCount() const
{
    Q_D(const QAmqpChannel);
    return d->prefetchCount;
}

QAMQP::Error QAmqpChannel::error() const
{
    Q_D(const QAmqpChannel);
    return d->error;
}

QString QAmqpChannel::errorString() const
{
    Q_D(const QAmqpChannel);
    return d->errorString;
}

void QAmqpChannel::resume()
{
    Q_D(QAmqpChannel);
    d->flow(true);
}

#include "moc_qamqpchannel.cpp"
