#include <QCoreApplication>
#include <QEventLoop>
#include <QUuid>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

#include "fibonaccirpcclient.h"

FibonacciRpcClient::FibonacciRpcClient(QObject *parent)
    : QObject(parent),
      m_client(0),
      m_responseQueue(0),
      m_defaultExchange(0)
{
    m_client = new QAmqpClient(this);
    connect(m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
}

FibonacciRpcClient::~FibonacciRpcClient()
{
}

bool FibonacciRpcClient::connectToServer()
{
    QEventLoop loop;
    connect(this, SIGNAL(connected()), &loop, SLOT(quit()));
    m_client->connectToHost();
    loop.exec();

    return m_client->isConnected();
}

void FibonacciRpcClient::call(int number)
{
    qDebug() << " [x] Requesting fib(" << number << ")";
    m_correlationId = QUuid::createUuid().toString();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::ReplyTo, m_responseQueue->name());
    properties.insert(QAmqpMessage::CorrelationId, m_correlationId);

    m_defaultExchange->publish(QByteArray::number(number), "rpc_queue", properties);
}

void FibonacciRpcClient::clientConnected()
{
    m_responseQueue = m_client->createQueue();
    connect(m_responseQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
    connect(m_responseQueue, SIGNAL(messageReceived()), this, SLOT(responseReceived()));
    m_responseQueue->declare(QAmqpQueue::Exclusive | QAmqpQueue::AutoDelete);
    m_defaultExchange = m_client->createExchange();
}

void FibonacciRpcClient::queueDeclared()
{
    m_responseQueue->consume();
    Q_EMIT connected();
}

void FibonacciRpcClient::responseReceived()
{
    QAmqpMessage message = m_responseQueue->dequeue();
    if (message.property(QAmqpMessage::CorrelationId).toString() != m_correlationId) {
        // requeue message, it wasn't meant for us
        m_responseQueue->reject(message, true);
        return;
    }

    qDebug() << " [.] Got " << message.payload();
    qApp->quit();
}
