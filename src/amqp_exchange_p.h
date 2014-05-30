#ifndef amqp_exchange_p_h__
#define amqp_exchange_p_h__

#include "amqp_channel_p.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{

class ExchangePrivate: public ChannelPrivate
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miDeclare, 10),
        METHOD_ID_ENUM(miDelete, 20)
    };

    ExchangePrivate(Exchange *q);
    ~ExchangePrivate();

    void declare();
    void remove(bool ifUnused = true, bool noWait = true);

    void declareOk(const Frame::Method &frame);
    void deleteOk(const Frame::Method &frame);

    void publish(const QByteArray &message, const QString &key,
                 const QString &mimeType = QLatin1String("text/plain"),
                 const QVariantHash &headers = QVariantHash(),
                 const Exchange::MessageProperties &properties = Exchange::MessageProperties());

    QString type;
    Exchange::ExchangeOptions options;
    Frame::TableField arguments;

    bool _q_method(const Frame::Method &frame);
    void _q_disconnected();

    bool delayedDeclare;
    bool declared;

    Q_DECLARE_PUBLIC(Exchange)
};

} // namespace QAMQP

#endif // amqp_exchange_p_h__
