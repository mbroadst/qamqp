#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
using namespace QAMQP;

class TopicLogReceiver : public QObject
{
    Q_OBJECT
public:
    TopicLogReceiver(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start(const QStringList &bindingKeys) {
        m_bindingKeys = bindingKeys;
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Exchange *topic_logs = m_client.createExchange("topic_logs");
        connect(topic_logs, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        topic_logs->declare(Exchange::Topic);
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

        foreach (QString bindingKey, m_bindingKeys)
            temporaryQueue->bind("topic_logs", bindingKey);
        qDebug() << " [*] Waiting for logs. To exit press CTRL+C";
    }

    void queueBound() {
        Queue *temporaryQueue = qobject_cast<Queue*>(sender());
        if (!temporaryQueue)
            return;
        temporaryQueue->consume(Queue::coNoAck);
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
    QStringList m_bindingKeys;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList bindingKeys = app.arguments().mid(1);
    if (bindingKeys.isEmpty()) {
        qDebug("usage: %s [binding_key] ...", argv[0]);
        return 1;
    }

    TopicLogReceiver logReceiver;
    logReceiver.start(bindingKeys);
    return app.exec();
}

#include "main.moc"
