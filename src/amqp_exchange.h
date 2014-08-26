#ifndef amqp_exchange_h__
#define amqp_exchange_h__

#include "amqp_table.h"
#include "amqp_channel.h"
#include "amqp_message.h"

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

    enum PublishOption {
        poNoOptions = 0x0,
        poMandatory = 0x01,
        poImmediate = 0x02
    };
    Q_DECLARE_FLAGS(PublishOptions, PublishOption)

    enum RemoveOption {
        roForce = 0x0,
        roIfUnused = 0x01,
        roNoWait = 0x04
    };
    Q_DECLARE_FLAGS(RemoveOptions, RemoveOption)

    enum ExchangeOption {
        NoOptions = 0x0,
        Passive = 0x01,
        Durable = 0x02,
        AutoDelete = 0x04,
        Internal = 0x08,
        NoWait = 0x10
    };
    Q_DECLARE_FLAGS(ExchangeOptions, ExchangeOption)
    ExchangeOptions options() const;

    virtual ~Exchange();

    // AMQP Exchange
    void declare(ExchangeType type = Direct,
                 ExchangeOptions options = NoOptions,
                 const Table &args = Table());
    void declare(const QString &type = QLatin1String("direct"),
                 ExchangeOptions options = NoOptions,
                 const Table &args = Table());
    void remove(int options = roIfUnused|roNoWait);

    // AMQP Basic
    void publish(const QString &message, const QString &routingKey,
                 const Message::PropertyHash &properties = Message::PropertyHash(),
                 int publishOptions = poNoOptions);
    void publish(const QByteArray &message, const QString &routingKey, const QString &mimeType,
                 const Message::PropertyHash &properties = Message::PropertyHash(),
                 int publishOptions = poNoOptions);
    void publish(const QByteArray &message, const QString &routingKey,
                 const QString &mimeType, const Table &headers,
                 const Message::PropertyHash &properties = Message::PropertyHash(),
                 int publishOptions = poNoOptions);

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
