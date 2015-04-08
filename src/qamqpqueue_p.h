#ifndef QAMQPQUEUE_P_H
#define QAMQPQUEUE_P_H

#include <QQueue>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <QStringList>

#include "qamqpchannel_p.h"

class QAmqpQueuePrivate: public QAmqpChannelPrivate,
                         public QAmqpContentFrameHandler,
                         public QAmqpContentBodyFrameHandler
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miDeclare, 10),
        METHOD_ID_ENUM(miBind, 20),
        METHOD_ID_ENUM(miUnbind, 50),
        METHOD_ID_ENUM(miPurge, 30),
        METHOD_ID_ENUM(miDelete, 40)
    };

    /*!
     * A class representing the state of this queue's subscriptions to
     * a given exchange.
     */
    class SubscriptionState {
    public:
        /*! Exchange pointer, for checking state */
        QPointer<QAmqpExchange> exchange;
        /*! Presently subscribed topics */
        QSet<QString>   topics;
        /*! Topics that are to be bound */
        QSet<QString>   topicsToBind;
        /*! Topics that are to be unbound */
        QSet<QString>   topicsToUnbind;
    };

    enum QueueState {
        /*! Channel is closed */
        Q_CLOSED,
        /*! Queue is undeclared */
        Q_UNDECLARED,
        /*! Queue is declaring */
        Q_DECLARING,
        /*! Queue is declared */
        Q_DECLARED,
        /*! Queue is being removed */
        Q_REMOVING,
    };

    enum ConsumerState {
        /*! Queue is undeclared */
        C_UNDECLARED,
        /*! Queue is declared */
        C_DECLARED,
        /*! Consumation request started */
        C_REQUESTED,
        /*! Consumation in progress */
        C_CONSUMING,
        /*! Cancellation in progress */
        C_CANCELLING,
    };

    QAmqpQueuePrivate(QAmqpQueue *q);
    ~QAmqpQueuePrivate();

    void declare();
    virtual bool _q_method(const QAmqpMethodFrame &frame);

    // AMQP Queue method handlers
    void declareOk(const QAmqpMethodFrame &frame);
    void deleteOk(const QAmqpMethodFrame &frame);
    void purgeOk(const QAmqpMethodFrame &frame);
    void bindOk(const QAmqpMethodFrame &frame);
    void unbindOk(const QAmqpMethodFrame &frame);
    void consumeOk(const QAmqpMethodFrame &frame);

    // AMQP Basic method handlers
    virtual void _q_content(const QAmqpContentFrame &frame);
    virtual void _q_body(const QAmqpContentBodyFrame &frame);
    void deliver(const QAmqpMethodFrame &frame);
    void getOk(const QAmqpMethodFrame &frame);
    void cancelOk(const QAmqpMethodFrame &frame);

    QString type;
    int options;
    bool delayedDeclare;
    QueueState queueState;

    /*!
     * Queue binding state.  Exchange name to binding state.
     */
    QHash<QString, SubscriptionState> bindings;

    /*! Name of exchange in current bind/unbind operation */
    QString bindPendingExchange;
    /*! Name of key in current bind/unbind operation */
    QString bindPendingKey;

    /*!
     * Retrieve the state of an exchange's bindings.
     */
    SubscriptionState& getState(const QString& exchangeName);

    /*!
     * Reset the binding state.  This marks all presently "subscribed" exchanges
     * and keys as "to be subscribed" except those presently marked as "to be
     * unsubscribed".
     */
    void resetBindings();

    /*!
     * Process binding list.
     */
    void processBindings();

    /*! Report and change state. */
    virtual void newState(ChannelState state);
    virtual void newState(QueueState state);
    virtual void newState(ConsumerState state);

    QString consumerTag;
    bool receivingMessage;
    QAmqpMessage currentMessage;
    int consumeOptions;
    ConsumerState consumerState;
    bool delayedConsume;

    Q_DECLARE_PUBLIC(QAmqpQueue)

};

QDebug operator<<(QDebug dbg, QAmqpQueuePrivate::QueueState s);
QDebug operator<<(QDebug dbg, QAmqpQueuePrivate::ConsumerState s);

#endif // QAMQPQUEUE_P_H
