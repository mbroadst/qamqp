#ifndef RECEIVE_H
#define RECEIVE_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QTimer>
#include <QDateTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_queue.h"


namespace QAMQP
{

namespace samples
{

class Receive : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit Receive(const QString& address, QObject* parent)
        : super(parent)
    {
        QAMQP::Client* client = new QAMQP::Client(this);
        client->open(QUrl(address));

        queue_ = client->createQueue("hello");
        queue_->declare();
        connect(queue_, SIGNAL(declared()), this, SLOT(declared()));
        connect(queue_, SIGNAL(messageReceived()), this, SLOT(newMessage()));
    }

    void run()
    {
    }

protected slots:
    void declared()
    {
        queue_->consume();
    }

    void newMessage()
    {
        while (queue_->hasMessage())
        {
            QAMQP::MessagePtr message = queue_->getMessage();
            qDebug() << "Receive::newMessage " << message->payload;
            if(!queue_->noAck())
            {
                queue_->ack(message);
            }
        }
    }

private:
    QAMQP::Queue* queue_;
};

}

}

#endif // RECEIVE_H
