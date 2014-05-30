#ifndef amqp_queue_p_h__
#define amqp_queue_p_h__

#include <QQueue>
#include "amqp_channel_p.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{

class QueuePrivate: public ChannelPrivate
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miDeclare, 10),
        METHOD_ID_ENUM(miBind, 20),
        METHOD_ID_ENUM(miUnbind, 50),
        METHOD_ID_ENUM(miPurge, 30),
        METHOD_ID_ENUM(miDelete, 40)
    };

    QueuePrivate(Queue *q);
    ~QueuePrivate();

    void declare();
    void remove(bool ifUnused = true, bool ifEmpty = true, bool noWait = true);
    void bind(const QString &exchangeName, const QString &key);
    void unbind(const QString &exchangeName, const QString &key);

    void declareOk(const Frame::Method &frame);
    void deleteOk(const Frame::Method &frame);
    void bindOk(const Frame::Method &frame);
    void unbindOk(const Frame::Method &frame);

    /************************************************************************/
    /* CLASS BASIC METHODS                                                  */
    /************************************************************************/

    void consume(Queue::ConsumeOptions options);
    void consumeOk(const Frame::Method &frame);
    void deliver(const Frame::Method &frame);

    void get();
    void getOk(const Frame::Method &frame);
    void ack(const MessagePtr &Message);

    QString type;
    Queue::QueueOptions options;

    bool _q_method(const Frame::Method &frame);

    bool delayedDeclare;
    bool declared;
    bool noAck;
    QString consumerTag;

    QQueue<QPair<QString, QString> > delayedBindings;
    QQueue<MessagePtr> messages_;

    bool recievingMessage;

    Q_DECLARE_PUBLIC(Queue)

};

} // namespace QAMQP

#endif // amqp_queue_p_h__
