#ifndef QAMQPEXCHANGE_P_H
#define QAMQPEXCHANGE_P_H

#include "qamqptable.h"
#include "qamqpexchange.h"
#include "qamqpchannel_p.h"

class QAmqpExchangePrivate: public QAmqpChannelPrivate
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miDeclare, 10),
        METHOD_ID_ENUM(miDelete, 20)
    };

    QAmqpExchangePrivate(QAmqpExchange *q);
    static QString typeToString(QAmqpExchange::ExchangeType type);

    void declare();

    // method handler related
    virtual void _q_disconnected();
    virtual bool _q_method(const QAmqpMethodFrame &frame);
    void declareOk(const QAmqpMethodFrame &frame);
    void deleteOk(const QAmqpMethodFrame &frame);
    void basicReturn(const QAmqpMethodFrame &frame);

    QString type;
    QAmqpExchange::ExchangeOptions options;
    QAmqpTable arguments;
    bool delayedDeclare;
    bool declared;

    Q_DECLARE_PUBLIC(QAmqpExchange)
};

#endif // QAMQPEXCHANGE_P_H
