#ifndef FIBONACCIRPCCLIENT_H
#define FIBONACCIRPCCLIENT_H

#include <QObject>

class QAmqpQueue;
class QAmqpExchange;
class QAmqpClient;
class FibonacciRpcClient : public QObject
{
    Q_OBJECT
public:
    explicit FibonacciRpcClient(QObject *parent = 0);
    ~FibonacciRpcClient();

Q_SIGNALS:
    void connected();

public Q_SLOTS:
    bool connectToServer();
    void call(int number);

private Q_SLOTS:
    void clientConnected();
    void queueDeclared();
    void responseReceived();

private:
    QAmqpClient *m_client;
    QAmqpQueue *m_responseQueue;
    QAmqpExchange *m_defaultExchange;
    QString m_correlationId;

};

#endif  // FIBONACCIRPCCLIENT_H
