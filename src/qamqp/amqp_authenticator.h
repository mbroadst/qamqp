#ifndef amqp_authenticator_h__
#define amqp_authenticator_h__

#include <QString>
#include <QDataStream>

#include "amqp_global.h"

namespace QAMQP
{

class Authenticator
{
public:
    virtual ~Authenticator() {}
    virtual QString type() const = 0;
    virtual void write(QDataStream &out) = 0;
};

class AMQPlainAuthenticator : public Authenticator
{
public:
    AMQPlainAuthenticator(const QString &login = QString(), const QString &password = QString());
    virtual ~AMQPlainAuthenticator();

    QString login() const;
    void setLogin(const QString &l);

    QString password() const;
    void setPassword(const QString &p);

    virtual QString type() const;
    virtual void write(QDataStream &out);

private:
    QString login_;
    QString password_;

};

}
#endif // amqp_authenticator_h__
