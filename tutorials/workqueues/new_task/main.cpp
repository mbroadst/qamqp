#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

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
        QAmqpQueue *queue = m_client.createQueue("task_queue");
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        queue->declare();
    }

    void queueDeclared() {
        QAmqpQueue *queue = qobject_cast<QAmqpQueue*>(sender());
        if (!queue)
            return;

        QAmqpExchange *defaultExchange = m_client.createExchange();
        QAmqpMessage::PropertyHash properties;
        properties[QAmqpMessage::DeliveryMode] = "2";   // make message persistent

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
    QAmqpClient m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    TaskCreator taskCreator;
    taskCreator.start();
    return app.exec();
}

#include "main.moc"
