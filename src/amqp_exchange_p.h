#ifndef amqp_exchange_p_h__
#define amqp_exchange_p_h__

#include "amqp_exchange.h"
#include "amqp_channel_p.h"

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
    static QString typeToString(Exchange::ExchangeType type);

    void declare();

    // method handler related
    virtual void _q_disconnected();
    virtual bool _q_method(const Frame::Method &frame);
    void declareOk(const Frame::Method &frame);
    void deleteOk(const Frame::Method &frame);

    QString type;
    Exchange::ExchangeOptions options;
    Frame::TableField arguments;
    bool delayedDeclare;
    bool declared;

    Q_DECLARE_PUBLIC(Exchange)
};

} // namespace QAMQP

#endif // amqp_exchange_p_h__
