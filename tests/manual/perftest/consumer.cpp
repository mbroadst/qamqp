#include <QSignalSpy>
#include <QTimer>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"
#include "consumer.h"

Consumer::Consumer(const QString &id, const TestOptions &options)
    : m_options(options),
      m_id(id)
{
}

Consumer::~Consumer()
{
}

void Consumer::run()
{
    QAmqpClient client;
    client.connectToHost(QHostAddress::LocalHost);
    if (!waitForSignal(&client, SIGNAL(connected()))) {
        qDebug() << "couldn't connect to host";
        return;
    }

    QAmqpExchange *exchange = client.createExchange(m_options.exchangeName);
    if (!m_options.predeclared || !exchangeExists(&client, m_options.exchangeName)) {
        exchange->declare(m_options.exchangeType);
        if (!waitForSignal(exchange, SIGNAL(declared()))) {
            qDebug() << "could not declare exchange";
            return;
        }
    }

    QAmqpQueue *queue = client.createQueue(m_options.queueName);
    if (!m_options.predeclared || !queueExists(&client, m_options.queueName)) {
        queue->declare(QAmqpQueue::AutoDelete);
        if (!waitForSignal(queue, SIGNAL(declared()))) {
            qDebug() << "could not declare queue";
            return;
        }
    }

    queue->bind(m_options.exchangeName, m_id);
    if (!waitForSignal(queue, SIGNAL(bound()))) {
        qDebug() << "could not bind queue";
        return;
    }
}

bool Consumer::exchangeExists(QAmqpClient *client, const QString &exchangeName)
{
    QEventLoop loop;
    QAmqpExchange *exchange = client->createExchange(exchangeName);
    connect(exchange, SIGNAL(declared()), &loop, SLOT(quit()));
    connect(exchange, SIGNAL(error(QAMQP::Error)), &loop, SLOT(quit()));
    QSignalSpy errorSpy(exchange, SIGNAL(error(QAMQP::Error)));
    exchange->declare("", QAmqpExchange::Passive);
    loop.exec();

    exchange->deleteLater();
    if (errorSpy.isEmpty())
        return true;
    return false;
}

bool Consumer::queueExists(QAmqpClient *client, const QString &queueName)
{
    QEventLoop loop;
    QAmqpQueue *queue = client->createQueue(queueName);
    connect(queue, SIGNAL(declared()), &loop, SLOT(quit()));
    connect(queue, SIGNAL(error(QAMQP::Error)), &loop, SLOT(quit()));
    QSignalSpy errorSpy(queue, SIGNAL(error(QAMQP::Error)));
    queue->declare(QAmqpQueue::Passive);
    loop.exec();

    queue->deleteLater();
    if (errorSpy.isEmpty())
        return true;
    return false;
}

bool Consumer::waitForSignal(QObject *obj, const char *signal, int delay)
{
    QEventLoop loop;
    QObject::connect(obj, signal, &loop, SLOT(quit()));
    QSignalSpy spy(obj, signal);
    QTimer::singleShot(delay, &loop, SLOT(quit()));
    loop.exec();
    return (spy.size() == 1);
}

