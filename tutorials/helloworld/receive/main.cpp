#include <QCoreApplication>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver(QObject *parent = 0) : QObject(parent) {
      m_client.setAutoReconnect(true);
    }

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        QAmqpQueue *queue = m_client.createQueue("hello");
        disconnect(queue, 0, 0, 0); // in case this is a reconnect
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        queue->declare();
    }

    void queueDeclared() {
        QAmqpQueue *queue = qobject_cast<QAmqpQueue*>(sender());
        if (!queue)
            return;

        connect(queue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        queue->consume(QAmqpQueue::coNoAck);
        qDebug() << " [*] Waiting for messages. To exit press CTRL+C";
    }

    void messageReceived() {
        QAmqpQueue *queue = qobject_cast<QAmqpQueue*>(sender());
        if (!queue)
            return;

        QAmqpMessage message = queue->dequeue();
        qDebug() << " [x] Received " << message.payload();
    }

private:
    QAmqpClient m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Receiver receiver;
    receiver.start();
    return app.exec();
}

#include "main.moc"
