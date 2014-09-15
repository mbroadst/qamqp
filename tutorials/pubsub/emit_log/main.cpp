#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

class LogEmitter : public QObject
{
    Q_OBJECT
public:
    LogEmitter(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        connect(&m_client, SIGNAL(disconnected()), qApp, SLOT(quit()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        QAmqpExchange *exchange = m_client.createExchange("logs");
        connect(exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        exchange->declare(QAmqpExchange::FanOut);
    }

    void exchangeDeclared() {
        QAmqpExchange *exchange = qobject_cast<QAmqpExchange*>(sender());
        if (!exchange)
            return;

        QString message;
        if (qApp->arguments().size() < 2)
            message = "info: Hello World!";
        else
            message = qApp->arguments().at(1);
        exchange->publish(message, "");
        qDebug() << " [x] Sent " << message;
        m_client.disconnectFromHost();
    }

private:
    QAmqpClient m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    LogEmitter logEmitter;
    logEmitter.start();
    return app.exec();
}

#include "main.moc"
