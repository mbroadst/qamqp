#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include "Issue23.h"
#include "qamqpclient.h"
#include "qamqpqueue.h"
#include "qamqpexchange.h"

const QByteArray Issue23Test::MSG_PAYLOAD("Hello AMQP World\n");

Issue23Test::Issue23Test(QAmqpClient* client, QObject* parent)
: QObject(parent), client(client), default_ex(NULL), queue(NULL),
  consumers(0), sent(0), passes(0), failures(0), remove_attempted(false)
{
    qDebug()    << "INIT: Creating connection";
    this->timer = new QTimer();
    this->timer->setSingleShot(true);
    connect(this->timer, SIGNAL(timeout()),
            this,        SLOT(timerTimeout()));
}

void Issue23Test::run()
{
    /* Give ourselves 2 seconds to create the queue */
    this->timer->start(2000);

    qDebug()    << "INIT: Creating exchange";
    this->timer->stop();
    this->default_ex    = this->client->createExchange();
    qDebug()    << "INIT: Creating queue";
    this->queue        = this->client->createQueue();
    connect(this->queue, SIGNAL(declared()),
            this,        SLOT(queueDeclared()));
    connect(this->queue, SIGNAL(removed()),
            this,        SLOT(queueRemoved()));
    connect(this->queue, SIGNAL(consuming(const QString&)),
            this,        SLOT(queueConsuming(const QString&)));
    connect(this->queue, SIGNAL(cancelled(const QString&)),
            this,        SLOT(queueCancelled(const QString&)));
    connect(this->queue, SIGNAL(messageReceived()),
            this,        SLOT(queueMessageReceived()));
    qDebug()    << "INIT: Declaring queue";
    this->queue->declare(QAmqpQueue::Exclusive);
}

Issue23Test::~Issue23Test()
{
    qDebug()    << "END: Disconnecting from queue";
    if (this->queue != NULL)
        disconnect(this->queue, 0, this, 0);
}

/*!
 * From signal QAmqpQueue::declared, consume the queue
 * multiple times.
 */
void Issue23Test::queueDeclared()
{
    qDebug()    << "DECLARED: Queue declared, named "
                << this->queue->name()
                << ", creating multiple consumers.";
    int count = 5;
    this->timer->stop();
    this->timer->start(2000);
    while(count-- > 0)
        if (this->queue->consume())
            this->consumers++;
        else
            break;

    qDebug()    << "DECLARED:"
                << this->consumers
                << "consumers started";
}

/*!
 * From signal QAmqpQueue::consuming, send a test message
 * to the queue.
 */
void Issue23Test::queueConsuming(const QString& consumer_tag)
{
    int count=3;
    qDebug()    << "CONSUMING: ConsumeOk received with tag "
                << consumer_tag
                << " sending test messages.";
    this->timer->stop();
    this->timer->start(2000);
    while(count-- > 0) {
        this->default_ex->publish(Issue23Test::MSG_PAYLOAD,
            this->queue->name(), "text/plain");
        this->sent++;
    }
    qDebug()    << "CONSUMING: Sent"
                << this->sent
                << "messages.";
}

/*!
 * From signal QAmqpQueue::cancelled, remove the queue.
 */
void Issue23Test::queueCancelled(const QString& consumer_tag)
{
    qDebug()    << "CANCELLED: Consumer" << consumer_tag;
    this->timer->stop();
    this->tryRemove();
}

/*!
 * from signal QAmqpQueue::messageReceived, grab the test
 * message and inspect its content.
 */
void Issue23Test::queueMessageReceived()
{
    const QAmqpMessage msg(this->queue->dequeue());
    qDebug()    << "RECEIVED: Got message, payload:"
                << msg.payload();
    this->timer->stop();
    this->timer->start(2000);
    if (msg.payload() == Issue23Test::MSG_PAYLOAD) {
        this->passes++;
        qDebug() << "RECEIVED: pass";
    } else {
        this->failures++;
        qDebug() << "RECEIVED: fail";
    }
}

/*!
 * from signal QAmqpQueue::removed, shut down.
 */
void Issue23Test::queueRemoved()
{
    qDebug()    << "REMOVED: Emitting results";
    this->queue = NULL;
    this->reportResults();
}

/*!
 * from signal QTimer::timeout, disconnect or shut down.
 */
void Issue23Test::timerTimeout()
{
    qDebug()    << "TIMEOUT: Time is up";
    if ((this->queue != NULL) 
            && this->queue->isDeclared()) {
        if (!this->queue->isConsuming()) {
            qDebug() << "TIMEOUT: Cancelling consumer...";
            this->timer->start(2000);
            this->queue->cancel();
            return;
        }

        if (!this->remove_attempted) {
            qDebug() << "TIMEOUT: Removing queue...";
            this->tryRemove();
            return;
        }
    }
    qDebug()    << "TIMEOUT: Emitting results";
    this->reportResults();
}

/*! Try to remove the queue */
void Issue23Test::tryRemove()
{
    qDebug()    << "TRYREMOVE: Attempting deletion";
    this->timer->start(2000);
    this->remove_attempted = true;
    this->queue->remove();
}

/*! Report the results */
void Issue23Test::reportResults()
{
    qDebug()    << "COMPLETE: We created"
                << this->consumers
                << "consumers, sent"
                << this->sent
                << "messages, received"
                << this->passes
                << "good messages and"
                << this->failures
                << "bad messages.";
    emit testComplete(this->consumers, this->sent, this->passes,
            this->failures);
}
