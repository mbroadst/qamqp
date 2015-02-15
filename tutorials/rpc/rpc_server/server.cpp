#include "qamqpclient.h"
#include "qamqpqueue.h"
#include "qamqpexchange.h"

#include "server.h"

Server::Server(QObject *parent)
    : QObject(parent),
      m_client(0),
      m_rpcQueue(0),
      m_defaultExchange(0)
{
    m_client = new QAmqpClient(this);
    connect(m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
}

Server::~Server()
{
}

void Server::listen()
{
    m_client->connectToHost();
}

int Server::fib(int n)
{
    if (n == 0)
        return 0;

    if (n == 1)
        return 1;

    return fib(n - 1) + fib(n - 2);
}

void Server::clientConnected()
{
    m_rpcQueue = m_client->createQueue("rpc_queue");
    connect(m_rpcQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
    connect(m_rpcQueue, SIGNAL(qosDefined()), this, SLOT(qosDefined()));
    connect(m_rpcQueue, SIGNAL(messageReceived()), this, SLOT(processRpcMessage()));
    m_rpcQueue->declare();

    m_defaultExchange = m_client->createExchange();
}

void Server::queueDeclared()
{
    m_rpcQueue->qos(1);
}

void Server::qosDefined()
{
    m_rpcQueue->consume();
    qDebug() << " [x] Awaiting RPC requests";
}

void Server::processRpcMessage()
{
    QAmqpMessage rpcMessage = m_rpcQueue->dequeue();
    int n = rpcMessage.payload().toInt();

    int response = fib(n);
    m_rpcQueue->ack(rpcMessage);

    QString replyTo = rpcMessage.property(QAmqpMessage::ReplyTo).toString();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::CorrelationId, rpcMessage.property(QAmqpMessage::CorrelationId));
    m_defaultExchange->publish(QByteArray::number(response), replyTo, properties);
}
