#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QTime>

#include "amqp_client.h"
#include "amqp_queue.h"

namespace QAMQP
{

namespace samples
{

class Worker : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit Worker(const QString& address, QObject* parent)
        : super(parent)
    {
        QAMQP::Client* client = new QAMQP::Client(this);
        client->connectToHost(address);

        queue_ = client->createQueue("task_queue");
        queue_->declare(Queue::Durable);
        connect(queue_, SIGNAL(declared()), this, SLOT(declared()));
        connect(queue_, SIGNAL(messageReceived()), this, SLOT(newMessage()));
    }

    void run()
    {
    }

protected slots:
    void declared()
    {
        queue_->setQOS(0,1);
        queue_->consume();
    }

    void newMessage()
    {
        // Retrieve message
        QAMQP::Message message = queue_->getMessage();
        qDebug() << "Worker::newMessage " << message.payload();

        // Simulate long processing
        int wait = message.payload().count('.');
        QTime dieTime = QTime::currentTime().addMSecs(400 * wait);
        while( QTime::currentTime() < dieTime );

        // Ack to server
        queue_->ack(message);
    }

private:
    QAMQP::Queue* queue_;
};

}

}

#endif // WORKER_H
