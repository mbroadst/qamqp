#ifndef QAMQPEXCHANGE_P_H
#define QAMQPEXCHANGE_P_H

#include "qamqptable.h"
#include "qamqpexchange.h"
#include "qamqpchannel_p.h"

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
    void basicReturn(const Frame::Method &frame);

    QString type;
    Exchange::ExchangeOptions options;
    Table arguments;
    bool delayedDeclare;
    bool declared;

    Q_DECLARE_PUBLIC(Exchange)
};

} // namespace QAMQP

#endif // QAMQPEXCHANGE_P_H
