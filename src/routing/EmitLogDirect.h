#ifndef EMITLOGDIRECT_H
#define EMITLOGDIRECT_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <QDateTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_exchange.h"

namespace QAMQP
{

namespace samples
{

class EmitLogDirect : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit EmitLogDirect(const QString& address, const QString& list, QObject *parent)
        : super(parent)
        , list_(list.split(',', QString::SkipEmptyParts))
    {
        // Create AMQP client
        QAMQP::Client* client = new QAMQP::Client(this);
        client->open(QUrl(address));

        // Create the "direct_logs" fanout exchange
        exchange_ =  client->createExchange("direct_logs");
        exchange_->declare("direct");
    }

    void run()
    {
        QTimer* timer = new QTimer(this);
        timer->setInterval(1000);
        connect(timer, SIGNAL(timeout()), SLOT(emitLogMessage()));
        timer->start();
    }

protected slots:
    void emitLogMessage()
    {
        static quint64 counter = 0;

        // Choose random key
        QString key(list_.at(qrand() % list_.size()));

        // Create Message
        QString message(QString("[%1: %2] %3")
          .arg(++counter)
          .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
          .arg(key)
          );
        qDebug() << "EmitLogDirect::emitLogMessage " << message;

        // Publish
        exchange_->publish(message, key);
    }

private:
    QStringList list_;
    QAMQP::Exchange* exchange_;
};

}

}

#endif // EMITLOGDIRECT_H
