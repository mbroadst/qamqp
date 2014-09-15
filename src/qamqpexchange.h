#ifndef QAMQPEXCHANGE_H
#define QAMQPEXCHANGE_H

#include "qamqptable.h"
#include "qamqpchannel.h"
#include "qamqpmessage.h"

class QAmqpClient;
class QAmqpQueue;
class QAmqpClientPrivate;
class QAmqpExchangePrivate;
class QAMQP_EXPORT QAmqpExchange : public QAmqpChannel
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

    virtual ~QAmqpExchange();

    // AMQP Exchange
    void declare(ExchangeType type = Direct,
                 ExchangeOptions options = NoOptions,
                 const QAmqpTable &args = QAmqpTable());
    void declare(const QString &type = QLatin1String("direct"),
                 ExchangeOptions options = NoOptions,
                 const QAmqpTable &args = QAmqpTable());
    void remove(int options = roIfUnused|roNoWait);

    // AMQP Basic
    void publish(const QString &message, const QString &routingKey,
                 const QAmqpMessage::PropertyHash &properties = QAmqpMessage::PropertyHash(),
                 int publishOptions = poNoOptions);
    void publish(const QByteArray &message, const QString &routingKey, const QString &mimeType,
                 const QAmqpMessage::PropertyHash &properties = QAmqpMessage::PropertyHash(),
                 int publishOptions = poNoOptions);
    void publish(const QByteArray &message, const QString &routingKey,
                 const QString &mimeType, const QAmqpTable &headers,
                 const QAmqpMessage::PropertyHash &properties = QAmqpMessage::PropertyHash(),
                 int publishOptions = poNoOptions);

Q_SIGNALS:
    void declared();
    void removed();

protected:
    virtual void channelOpened();
    virtual void channelClosed();

private:
    explicit QAmqpExchange(int channelNumber = -1, QAmqpClient *parent = 0);

    Q_DISABLE_COPY(QAmqpExchange)
    Q_DECLARE_PRIVATE(QAmqpExchange)

    friend class QAmqpClient;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAmqpExchange::ExchangeOptions)
Q_DECLARE_METATYPE(QAmqpExchange::ExchangeType)

#endif // QAMQPEXCHANGE_H
