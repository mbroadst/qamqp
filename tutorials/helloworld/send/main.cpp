#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

class Sender : public QObject
{
    Q_OBJECT
public:
    Sender(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        connect(&m_client, SIGNAL(disconnected()), qApp, SLOT(quit()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        QAmqpQueue *queue = m_client.createQueue("hello");
        connect(queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        queue->declare();
    }

    void queueDeclared() {
        QAmqpQueue *queue = qobject_cast<QAmqpQueue*>(sender());
        if (!queue)
            return;
        QAmqpExchange *defaultExchange = m_client.createExchange();
        defaultExchange->publish("Hello World!", "hello");
        qDebug() << " [x] Sent 'Hello World!'";
        m_client.disconnectFromHost();
    }

private:
    QAmqpClient m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Sender sender;
    sender.start();
    return app.exec();
}

#include "main.moc"
