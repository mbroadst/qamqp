#ifndef amqp_channel_h__
#define amqp_channel_h__

#include <QObject>
#include "amqp_global.h"
#include "amqp_frame.h"

namespace QAMQP
{

class Client;
class ChannelPrivate;
class Channel : public QObject, public Frame::MethodHandler
{
    Q_OBJECT
    Q_PROPERTY(int number READ channelNumber)
    Q_PROPERTY(QString name READ name WRITE setName)

public:
    ~Channel();

    void closeChannel();
    void reopen();

    QString name() const;
    int channelNumber() const;

    void setName(const QString &name);
    void setQOS(qint32 prefetchSize, quint16 prefetchCount);
    bool isOpened() const;

signals:
    void opened();
    void closed();
    void flowChanged(bool enabled);

protected:
    Q_DISABLE_COPY(Channel)
    Q_DECLARE_PRIVATE(QAMQP::Channel)

    Channel(int channelNumber = -1, Client *parent = 0);
    Channel(ChannelPrivate *dd, Client *parent = 0);
    QScopedPointer<ChannelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_open())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())

    virtual void onOpen();
    virtual void onClose();
    void stateChanged(int state);
    void _q_method(const QAMQP::Frame::Method &frame);

    friend class ClientPrivate;
};

}

#endif
