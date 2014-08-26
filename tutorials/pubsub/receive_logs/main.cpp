#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
using namespace QAMQP;

class LogReceiver : public QObject
{
    Q_OBJECT
public:
    LogReceiver(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Exchange *exchange = m_client.createExchange("logs");
        connect(exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        exchange->declare(Exchange::FanOut);
    }

    void exchangeDeclared() {
        Queue *temporaryQueue = m_client.createQueue();
        connect(temporaryQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        connect(temporaryQueue, SIGNAL(bound()), this, SLOT(queueBound()));
        connect(temporaryQueue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        temporaryQueue->declare(Queue::Exclusive);
    }

    void queueDeclared() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;

        temporaryQueue->bind("logs", temporaryQueue->name());
    }

    void queueBound() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;

        qDebug() << " [*] Waiting for logs. To exit press CTRL+C";
        temporaryQueue->consume(Queue::coNoAck);
    }

    void messageReceived() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;

        Message message = temporaryQueue->dequeue();
        qDebug() << " [x] " << message.payload();
    }

private:
    Client m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    LogReceiver logReceiver;
    logReceiver.start();
    return app.exec();
}

#include "main.moc"
