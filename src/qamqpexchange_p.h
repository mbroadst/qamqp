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

    enum ConfirmMethod {
        METHOD_ID_ENUM(cmConfirm, 10)
    };

    enum ExchangeState {
        /*! Exchange channel is closed */
        ExchangeClosedState,
        /*! Exchange is undeclared */
        ExchangeUndeclaredState,
        /*! Exchange is being declared */
        ExchangeDeclaringState,
        /*! Exchange is declared */
        ExchangeDeclaredState,
        /*! Exchange is being removed */
        ExchangeRemovingState,
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
    void handleAckOrNack(const QAmqpMethodFrame &frame);

    QString type;
    QAmqpExchange::ExchangeOptions options;
    int removeOptions;
    QAmqpTable arguments;
    bool delayedDeclare;
    bool delayedRemove;
    ExchangeState exchangeState;
    qlonglong nextDeliveryTag;
    QVector<qlonglong> unconfirmedDeliveryTags;

    /*! Report and change state. */
    virtual void newState(ChannelState state);
    virtual void newState(ExchangeState state);

    Q_DECLARE_PUBLIC(QAmqpExchange)
};

QDebug operator<<(QDebug dbg, QAmqpExchangePrivate::ExchangeState s);

#endif // QAMQPEXCHANGE_P_H
