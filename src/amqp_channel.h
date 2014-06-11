#ifndef amqp_channel_h__
#define amqp_channel_h__

#include <QObject>
#include "amqp_frame.h"

namespace QAMQP
{

class Client;
class ChannelPrivate;
class QAMQP_EXPORT Channel : public QObject
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

    Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void closeChannel();
    void reopen();
    void setQOS(qint32 prefetchSize, quint16 prefetchCount);

Q_SIGNALS:
    void opened();
    void closed();
    void flowChanged(bool enabled);
    void error(QAMQP::Error error);

protected:
    virtual void channelOpened() = 0;
    virtual void channelClosed() = 0;

protected:
    explicit Channel(int channelNumber = -1, Client *client = 0);
    explicit Channel(ChannelPrivate *dd, Client *client);

    Q_DISABLE_COPY(Channel)
    Q_DECLARE_PRIVATE(Channel)
    QScopedPointer<ChannelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_open())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())

    friend class ClientPrivate;
};

} // namespace QAMQP

#endif
