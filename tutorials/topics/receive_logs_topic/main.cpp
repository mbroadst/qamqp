#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

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
        QAmqpExchange *topic_logs = m_client.createExchange("topic_logs");
        connect(topic_logs, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        topic_logs->declare(QAmqpExchange::Topic);
    }

    void exchangeDeclared() {
        QAmqpQueue *temporaryQueue = m_client.createQueue();
        connect(temporaryQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        connect(temporaryQueue, SIGNAL(bound()), this, SLOT(queueBound()));
        connect(temporaryQueue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        temporaryQueue->declare(QAmqpQueue::Exclusive);
    }

    void queueDeclared() {
        QAmqpQueue *temporaryQueue = qobject_cast<QAmqpQueue*>(sender());
        if (!temporaryQueue)
            return;

        foreach (QString bindingKey, m_bindingKeys)
            temporaryQueue->bind("topic_logs", bindingKey);
        qDebug() << " [*] Waiting for logs. To exit press CTRL+C";
    }

    void queueBound() {
        QAmqpQueue *temporaryQueue = qobject_cast<QAmqpQueue*>(sender());
        if (!temporaryQueue)
            return;
        temporaryQueue->consume(QAmqpQueue::coNoAck);
    }

    void messageReceived() {
        QAmqpQueue *temporaryQueue = qobject_cast<QAmqpQueue*>(sender());
        if (!temporaryQueue)
            return;

        QAmqpMessage message = temporaryQueue->dequeue();
        qDebug() << " [x] " << message.routingKey() << ":" << message.payload();
    }

private:
    QAmqpClient m_client;
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
