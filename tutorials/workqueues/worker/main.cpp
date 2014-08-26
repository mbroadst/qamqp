#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpqueue.h"
using namespace QAMQP;

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker(QObject *parent = 0) : QObject(parent), m_queue(0) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        m_queue = m_client.createQueue("task_queue");
        connect(m_queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        m_queue->declare();
        qDebug() << " [*] Waiting for messages. To exit press CTRL+C";
    }

    void queueDeclared() {
        // m_queue->setPrefetchCount(1);
        m_queue->consume();
        connect(m_queue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
    }

    void messageReceived() {
        m_currentMessage = m_queue->dequeue();
        qDebug() << " [x] Received " << m_currentMessage.payload();

        int delay = m_currentMessage.payload().count(".") * 1000;
        QTimer::singleShot(delay, this, SLOT(ackMessage()));
    }

    void ackMessage() {
        qDebug() << " [x] Done";
        m_queue->ack(m_currentMessage);
    }

private:
    Client m_client;
    Queue *m_queue;
    Message m_currentMessage;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Worker worker;
    worker.start();
    return app.exec();
}

#include "main.moc"
