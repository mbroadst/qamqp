#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

class TopicLogEmitter : public QObject
{
    Q_OBJECT
public:
    TopicLogEmitter(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        connect(&m_client, SIGNAL(disconnected()), qApp, SLOT(quit()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        QAmqpExchange *topic_logs = m_client.createExchange("topic_logs");
        connect(topic_logs, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        topic_logs->declare(QAmqpExchange::Topic);
    }

    void exchangeDeclared() {
        QAmqpExchange *topic_logs = qobject_cast<QAmqpExchange*>(sender());
        if (!topic_logs)
            return;

        QStringList args = qApp->arguments();
        args.takeFirst();   // remove executable name

        QString routingKey = (args.isEmpty() ? "anonymous.info" : args.first());
        QString message;
        if (args.size() > 1) {
            args.takeFirst();
            message = args.join(" ");
        } else {
            message = "Hello World!";
        }

        topic_logs->publish(message, routingKey);
        qDebug(" [x] Sent %s:%s", routingKey.toLatin1().constData(), message.toLatin1().constData());
        m_client.disconnectFromHost();
    }

private:
    QAmqpClient m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    TopicLogEmitter logEmitter;
    logEmitter.start();
    return app.exec();
}

#include "main.moc"
