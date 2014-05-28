#ifndef amqp_channel_h__
#define amqp_channel_h__

#include <QObject>
#include "amqp_frame.h"

namespace QAMQP
{

class Client;
class ChannelPrivate;
class QAMQP_EXPORT Channel : public QObject, public Frame::MethodHandler
{
    Q_OBJECT
    Q_PROPERTY(int number READ channelNumber CONSTANT)
    Q_PROPERTY(bool opened READ isOpened CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName)
public:
    virtual ~Channel();

    int channelNumber() const;
    bool isOpened() const;

    QString name() const;
    void setName(const QString &name);

public Q_SLOTS:
    void closeChannel();
    void reopen();
    void setQOS(qint32 prefetchSize, quint16 prefetchCount);

Q_SIGNALS:
    void opened();
    void closed();
    void flowChanged(bool enabled);

protected:
    Channel(int channelNumber = -1, Client *parent = 0);
    Channel(ChannelPrivate *dd, Client *parent = 0);

    Q_DISABLE_COPY(Channel)
    Q_DECLARE_PRIVATE(Channel)
    QScopedPointer<ChannelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_open())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())

    // should move to private classes
    virtual void onOpen();
    virtual void onClose();
    void stateChanged(int state);
    void _q_method(const Frame::Method &frame);

    friend class ClientPrivate;
};

} // namespace QAMQP

#endif
