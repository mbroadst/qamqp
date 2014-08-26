#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"
using namespace QAMQP;

class TaskCreator : public QObject
{
    Q_OBJECT
public:
    TaskCreator(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        connect(&m_client, SIGNAL(disconnected()), qApp, SLOT(quit()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Queue *queue = m_client.createQueue("task_queue");
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        queue->declare();
    }

    void queueDeclared() {
        Queue *queue = qobject_cast<Queue*>(sender());
        if (!queue)
            return;

        Exchange *defaultExchange = m_client.createExchange();
        Message::PropertyHash properties;
        properties[Message::DeliveryMode] = "2";   // make message persistent

        QString message;
        if (qApp->arguments().size() < 2)
            message = "Hello World!";
        else
            message = qApp->arguments().at(1);

        defaultExchange->publish(message, "task_queue", properties);
        qDebug(" [x] Sent '%s'", message.toLatin1().constData());
        m_client.disconnectFromHost();
    }

private:
    Client m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    TaskCreator taskCreator;
    taskCreator.start();
    return app.exec();
}

#include "main.moc"
