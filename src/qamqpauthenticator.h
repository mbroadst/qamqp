#ifndef QAMQPAUTHENTICATOR_H
#define QAMQPAUTHENTICATOR_H

#include <QString>
#include <QDataStream>

#include "qamqpglobal.h"

namespace QAMQP
{

class QAMQP_EXPORT Authenticator
{
public:
    virtual ~Authenticator() {}
    virtual QString type() const = 0;
    virtual void write(QDataStream &out) = 0;
};

class QAMQP_EXPORT AMQPlainAuthenticator : public Authenticator
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

} // namespace QAMQP

#endif // QAMQPAUTHENTICATOR_H
