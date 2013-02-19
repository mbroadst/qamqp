#ifndef SEND_H
#define SEND_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QTimer>
#include <QDateTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_exchange.h"
#include "qamqp/amqp_queue.h"

namespace QAMQP
{

namespace samples
{

class Send : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit Send(const QString& address, const QString& sendMsg, QObject *parent)
        : super(parent), sendMsg_(sendMsg)
    {
        // Create AMQP client
        QAMQP::Client* client = new QAMQP::Client(this);
        client->open(QUrl(address));

        // Retrieve the "Default" exchange
        // No need to declare (i.e. to create), nor to bind to a queue
        exchange_ =  client->createExchange();

        // Create the "hello" queue
        // This isn't mandatory but if it doesn't exist, the messages are lost
        client
            ->createQueue("hello", exchange_->channelNumber())
            ->declare();
    }

    void run()
    {
        QTimer* timer = new QTimer(this);
        timer->setInterval(2468);
        connect(timer, SIGNAL(timeout()), SLOT(sendMessage()));
        timer->start();
    }

protected slots:
    void sendMessage()
    {
        static quint64 counter = 0;

        QString message(QString("[%1: %2] %3")
          .arg(++counter)
          .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
          .arg(sendMsg_));
        qDebug() << "Send::sendMessage " << message;

        exchange_->publish(message, "hello");
    }

private:
    QString sendMsg_;

    QAMQP::Exchange* exchange_;
};

}

}

#endif // SEND_H
