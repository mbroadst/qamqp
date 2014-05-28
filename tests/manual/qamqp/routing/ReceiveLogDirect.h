#ifndef RECEIVELOGDIRECT_H
#define RECEIVELOGDIRECT_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QStringList>
#include <QThread>
#include <QTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_queue.h"


namespace QAMQP
{

namespace samples
{

class ReceiveLogDirect : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit ReceiveLogDirect(const QString& address, const QString& list, QObject* parent)
        : super(parent)
        , list_(list)
    {
        // Create AMQP client
        QAMQP::Client* client = new QAMQP::Client(this);
        client->open(QUrl(address));

        // Create an exclusive queue
        queue_ = client->createQueue();
        queue_->declare("", Queue::Exclusive);

        connect(queue_, SIGNAL(declared()), this, SLOT(declared()));
        connect(queue_, SIGNAL(messageReceived()), this, SLOT(newMessage()));
    }

    void run()
    {
    }

protected slots:
    void declared()
    {
        // Loop on the list to bind with the keys
        QStringList split(list_.split(',', QString::SkipEmptyParts));
        for(int i = 0; i < split.size(); ++i)
            queue_->bind("direct_logs", split.at(i));

        // Start consuming
        queue_->consume(QAMQP::Queue::coNoAck);
    }

    void newMessage()
    {
        // Retrieve message
        QAMQP::MessagePtr message = queue_->getMessage();
        qDebug() << "ReceiveLogDirect::newMessage " << message->payload;
    }

private:
    QString list_;
    QAMQP::Queue* queue_;
};

}

}

#endif // RECEIVELOGDIRECT_H
