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
        QueueClosedState,
        /*! Queue is undeclared */
        QueueUndeclaredState,
        /*! Queue is declaring */
        QueueDeclaringState,
        /*! Queue is declared */
        QueueDeclaredState,
        /*! Queue is being removed */
        QueueRemovingState,
    };

    enum ConsumerState {
        /*! Queue is undeclared */
        ConsumerQueueUndeclaredState,
        /*! Queue is declared */
        ConsumerQueueDeclaredState,
        /*! Consumation request started */
        ConsumerRequestedState,
        /*! Consumation in progress */
        ConsumerConsumingState,
        /*! Cancellation in progress */
        ConsumerCancellingState,
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

    // Private slots

    /*! Process delayed bindings for a given exchange. */
    void _q_exchangeDeclared();

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
    SubscriptionState& getState(const QString& exchangeName,
		    QAmqpExchange* exchange=0);

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
