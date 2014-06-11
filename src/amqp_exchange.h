#ifndef amqp_exchange_h__
#define amqp_exchange_h__

#include "amqp_channel.h"

namespace QAMQP
{

class Client;
class Queue;
class ClientPrivate;
class ExchangePrivate;
class QAMQP_EXPORT Exchange : public Channel
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(ExchangeOptions options READ options CONSTANT)
    Q_ENUMS(ExchangeOptions)

public:
    enum ExchangeType {
        Direct,
        FanOut,
        Topic,
        Headers
    };
    QString type() const;

    enum RemoveOption {
        roForce = 0x0,
        roIfUnused = 0x1,
        roNoWait = 0x04
    };
    Q_DECLARE_FLAGS(RemoveOptions, RemoveOption)

    enum ExchangeOption {
        NoOptions = 0x0,
        Passive = 0x01,
        Durable = 0x02,
        AutoDelete = 0x4,
        Internal = 0x8,
        NoWait = 0x10
    };
    Q_DECLARE_FLAGS(ExchangeOptions, ExchangeOption)
    ExchangeOptions options() const;

    virtual ~Exchange();

    // AMQP Exchange
    void declare(ExchangeType type = Direct,
                 ExchangeOptions options = NoOptions,
                 const Frame::TableField &args = Frame::TableField());
    void declare(const QString &type = QLatin1String("direct"),
                 ExchangeOptions options = NoOptions,
                 const Frame::TableField &args = Frame::TableField());
    void remove(int options = roIfUnused|roNoWait);

    // AMQP Basic
    void publish(const QString &key, const QString &message,
                 const MessageProperties &properties = MessageProperties());
    void publish(const QString &key, const QByteArray &message,
                 const QString &mimeType, const MessageProperties &properties = MessageProperties());
    void publish(const QString &key, const QByteArray &message,
                 const QString &mimeType, const QVariantHash &headers,
                 const MessageProperties &properties = MessageProperties());

Q_SIGNALS:
    void declared();
    void removed();

protected:
    virtual void channelOpened();
    virtual void channelClosed();

private:
    explicit Exchange(int channelNumber = -1, Client *parent = 0);

    Q_DISABLE_COPY(Exchange)
    Q_DECLARE_PRIVATE(Exchange)

    friend class Client;

};

} // namespace QAMQP

Q_DECLARE_OPERATORS_FOR_FLAGS(QAMQP::Exchange::ExchangeOptions)
Q_DECLARE_METATYPE(QAMQP::Exchange::ExchangeType)

#endif // amqp_exchange_h__
