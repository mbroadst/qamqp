#ifndef SERVER_H
#define SERVER_H

#include <QObject>

class QAmqpQueue;
class QAmqpExchange;
class QAmqpClient;
class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);
    ~Server();

public Q_SLOTS:
    void listen();
    int fib(int n);

private Q_SLOTS:
    void clientConnected();
    void queueDeclared();
    void qosDefined();
    void processRpcMessage();

private:
    QAmqpClient *m_client;
    QAmqpQueue *m_rpcQueue;
    QAmqpExchange *m_defaultExchange;

};

#endif  // SERVER_H
