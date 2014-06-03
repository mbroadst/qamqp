#ifndef amqp_client_p_h__
#define amqp_client_p_h__

#include <QSharedPointer>
#include <QPointer>

namespace QAMQP
{

class Queue;
class Exchange;
class Network;
class Connection;
class Authenticator;
class ClientPrivate
{
public:
    ClientPrivate(Client *q);
    ~ClientPrivate();

    void init(const QUrl &connectionString = QUrl());
    void connect();
    void disconnect();
    void parseConnectionString(const QUrl &connectionString);

    quint32 port;
    QString host;
    QString virtualHost;

    QPointer<Network> network_;
    QPointer<Connection> connection_;
    QSharedPointer<Authenticator> auth_;

    bool isSSl() const;

    Client * const q_ptr;
    Q_DECLARE_PUBLIC(Client)

};

} // namespace QAMQP

#endif // amqp_client_p_h__