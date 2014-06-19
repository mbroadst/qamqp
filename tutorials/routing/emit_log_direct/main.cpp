#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include "amqp_client.h"
#include "amqp_exchange.h"
#include "amqp_queue.h"
using namespace QAMQP;

class DirectLogEmitter : public QObject
{
    Q_OBJECT
public:
    DirectLogEmitter(QObject *parent = 0) : QObject(parent) {}

public Q_SLOTS:
    void start() {
        connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
        connect(&m_client, SIGNAL(disconnected()), qApp, SLOT(quit()));
        m_client.connectToHost();
    }

private Q_SLOTS:
    void clientConnected() {
        Exchange *exchange = m_client.createExchange("direct_logs");
        connect(exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
        exchange->declare(Exchange::Direct);
    }

    void exchangeDeclared() {
        Exchange *exchange = qobject_cast<Exchange*>(sender());
        if (!exchange)
            return;

        QStringList args = qApp->arguments();
        args.takeFirst();   // remove executable name

        QString severity = (args.isEmpty() ? "info" : args.first());
        QString message;
        if (args.size() > 1) {
            args.takeFirst();
            message = args.join(" ");
        } else {
            message = "Hello World!";
        }

        exchange->publish(message, severity);
        qDebug(" [x] Sent %s:%s", severity.toLatin1().constData(), message.toLatin1().constData());
        m_client.disconnectFromHost();
    }

private:
    Client m_client;

};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    DirectLogEmitter logEmitter;
    logEmitter.start();
    return app.exec();
}

#include "main.moc"
