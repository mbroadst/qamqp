#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

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
        QAmqpExchange *exchange = m_client.createExchange("direct_logs");
        connect(exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        exchange->declare(QAmqpExchange::Direct);
    }

    void exchangeDeclared() {
        QAmqpQueue *temporaryQueue = m_client.createQueue();
        connect(temporaryQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
        connect(temporaryQueue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
        temporaryQueue->declare(QAmqpQueue::Exclusive);
    }

    void queueDeclared() {
        QAmqpQueue *temporaryQueue = qobject_cast<QAmqpQueue*>(sender());
        if (!temporaryQueue)
            return;

        // start consuming
        temporaryQueue->consume(QAmqpQueue::coNoAck);

        foreach (QString severity, m_severities)
            temporaryQueue->bind("direct_logs", severity);
        qDebug() << " [*] Waiting for logs. To exit press CTRL+C";
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
