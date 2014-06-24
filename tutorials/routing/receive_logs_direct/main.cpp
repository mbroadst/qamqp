#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"
using namespace QAMQP;

class DirectLogReceiver : public QObject
{
    Q_OBJECT
public:
    DirectLogReceiver(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start(const QStringList &severities) {
        m_severities = severities;
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Exchange *exchange = m_client.createExchange("direct_logs");
        connect(exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        exchange->declare(Exchange::Direct);
    }

    void exchangeDeclared() {
        Queue *temporaryQueue = m_client.createQueue();
        connect(temporaryQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        connect(temporaryQueue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        temporaryQueue->declare(Queue::Exclusive);
    }

    void queueDeclared() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;

        // start consuming
        temporaryQueue->consume(Queue::coNoAck);

        foreach (QString severity, m_severities)
            temporaryQueue->bind("direct_logs", severity);
        qDebug() << " [*] Waiting for logs. To exit press CTRL+C";
    }

    void messageReceived() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;

        Message message = temporaryQueue->dequeue();
        qDebug() << " [x] " << message.routingKey() << ":" << message.payload();
    }

private:
    Client m_client;
    QStringList m_severities;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList severities = app.arguments().mid(1);
    if (severities.isEmpty()) {
        qDebug("usage: %s [info] [warning] [error]", argv[0]);
        return 1;
    }

    DirectLogReceiver logReceiver;
    logReceiver.start(severities);
    return app.exec();
}

#include "main.moc"
