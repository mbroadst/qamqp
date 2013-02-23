#ifndef RECEIVELOG_H
#define RECEIVELOG_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_queue.h"


namespace QAMQP
{

namespace samples
{

class ReceiveLog : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit ReceiveLog(const QString& address, QObject* parent)
        : super(parent)
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
        // Bind the queue to the "logs" exchange (declared by publisher)
        queue_->bind("logs", "");
        queue_->consume(QAMQP::Queue::coNoAck);
    }

    void newMessage()
    {
        // Retrieve message
        QAMQP::MessagePtr message = queue_->getMessage();
        qDebug() << "ReceiveLog::newMessage " << message->payload;
    }

private:
    QAMQP::Queue* queue_;
};

}

}

#endif // RECEIVELOG_H
