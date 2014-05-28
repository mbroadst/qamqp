#ifndef amqp_exchange_h__
#define amqp_exchange_h__

#include "amqp_channel.h"

namespace QAMQP
{

using namespace QAMQP::Frame;

class Client;
class Queue;
class ClientPrivate;
class ExchangePrivate;
class Exchange : public Channel
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(ExchangeOptions option READ option)
    Q_ENUMS(ExchangeOption)

public:
    enum ExchangeOption {
        NoOptions = 0x0,
        Passive = 0x01,
        Durable = 0x02,
        AutoDelete = 0x4,
        Internal = 0x8,
        NoWait = 0x10
    };
    Q_DECLARE_FLAGS(ExchangeOptions, ExchangeOption)

    typedef QHash<QAMQP::Frame::Content::Property, QVariant> MessageProperties;

    virtual ~Exchange();

    QString type() const;
    ExchangeOptions option() const;

    void declare(const QString &type = QLatin1String("direct"),
                 ExchangeOptions option = NoOptions,
                 const TableField &arg = TableField());
    void remove(bool ifUnused = true, bool noWait = true);

    void bind(QAMQP::Queue *queue);
    void bind(const QString &queueName);
    void bind(const QString &queueName, const QString &key);

    void publish(const QString &message, const QString &key,
                 const MessageProperties &property = MessageProperties());
    void publish(const QByteArray &message, const QString &key,
                 const QString &mimeType, const MessageProperties &property = MessageProperties());
    void publish(const QByteArray &message, const QString &key, const QVariantHash &headers,
                 const QString &mimeType, const MessageProperties &property = MessageProperties());

Q_SIGNALS:
    void declared();
    void removed();

protected:
    void onOpen();
    void onClose();

private:
    Q_DISABLE_COPY(Exchange)
    Q_DECLARE_PRIVATE(QAMQP::Exchange)
    Exchange(int channelNumber = -1, Client * parent = 0);
    friend class ClientPrivate;

};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QAMQP::Exchange::ExchangeOptions)

#endif // amqp_exchange_h__
