#ifndef QAMQPQUEUE_P_H
#define QAMQPQUEUE_P_H

#include <QQueue>
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

    QAmqpQueuePrivate(QAmqpQueue *q);
    ~QAmqpQueuePrivate();

    virtual void resetInternalState();

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
    bool declared;
    QQueue<QPair<QString, QString> > delayedBindings;

    QString consumerTag;
    bool recievingMessage;
    QAmqpMessage currentMessage;
    bool consuming;
    bool consumeRequested;

    qint32 messageCount;
    qint32 consumerCount;

    Q_DECLARE_PUBLIC(QAmqpQueue)

};

#endif // QAMQPQUEUE_P_H
