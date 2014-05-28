#ifndef EMITLOG_H
#define EMITLOG_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

#include "qamqp/amqp.h"
#include "qamqp/amqp_exchange.h"

namespace QAMQP
{

namespace samples
{

class EmitLog : public QObject, public QRunnable
{
    Q_OBJECT

    typedef QObject super;

public:
    explicit EmitLog(const QString& address, QObject *parent)
        : super(parent)
    {
        // Create AMQP client
        QAMQP::Client* client = new QAMQP::Client(this);
        client->open(QUrl(address));

        // Create the "logs" fanout exchange
        exchange_ =  client->createExchange("logs");
        exchange_->declare("fanout");
    }

    void run()
    {
        QTimer* timer = new QTimer(this);
        timer->setInterval(1000);
        connect(timer, SIGNAL(timeout()), SLOT(emitlogMessage()));
        timer->start();
    }

protected slots:
    void emitlogMessage()
    {
        static quint64 counter = 0;

        QString message(QString("[%1: %2] Hello World!")
          .arg(++counter)
          .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
          );
        qDebug() << "EmitLog::emitlogMessage " << message;

        exchange_->publish(message, "");
    }

private:
    QAMQP::Exchange* exchange_;
};

}

}

#endif // EMITLOG_H
