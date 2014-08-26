#include <QCoreApplication>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
using namespace QAMQP;

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Queue *queue = m_client.createQueue("hello");
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        queue->declare();
    }

    void queueDeclared() {
        Queue *queue = qobject_cast<Queue*>(sender());
        if (!queue)
            return;

        connect(queue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        queue->consume(Queue::coNoAck);
        qDebug() << " [*] Waiting for messages. To exit press CTRL+C";
    }

    void messageReceived() {
        Queue *queue = qobject_cast<Queue*>(sender());
        if (!queue)
            return;

        Message message = queue->dequeue();
        qDebug() << " [x] Received " << message.payload();
    }

private:
    Client m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Receiver receiver;
    receiver.start();
    return app.exec();
}

#include "main.moc"
