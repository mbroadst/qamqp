#ifndef NEWTASK_H
#define NEWTASK_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"

namespace QAMQP
{

namespace samples
{

class NewTask : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit NewTask(const QString& address, QObject *parent)
        : super(parent)
    {
        // Create AMQP client
        QAMQP::Client* client = new QAMQP::Client(this);
        client->connectToHost(address);

        // Retrieve the "Default" exchange
        exchange_ =  client->createExchange();

        // Create the "task_queue" queue, with the "durable" option set
        queue_ = client->createQueue("task_queue", exchange_->channelNumber());
        queue_->declare(Queue::Durable);
    }

    void run()
    {
        QTimer* timer = new QTimer(this);
        timer->setInterval(1000);
        connect(timer, SIGNAL(timeout()), SLOT(newtaskMessage()));
        timer->start();
    }

protected slots:
    void newtaskMessage()
    {
        static quint64 counter = 0;

        QAMQP::MessageProperties properties;
        properties[QAMQP::Frame::Content::cpDeliveryMode] = 2; // Make message persistent

        QString message(QString("[%1: %2] Hello World! %3")
          .arg(++counter)
          .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
          .arg(QString('.').repeated(qrand() % 10)));
        qDebug() << "NewTask::newtaskMessage " << message;

        exchange_->publish(queue_->name(), message, properties);
    }

private:
    QAMQP::Exchange* exchange_;
    QAMQP::Queue* queue_;
};

}

}

#endif // NEWTASK_H
