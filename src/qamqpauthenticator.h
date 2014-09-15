#ifndef QAMQPAUTHENTICATOR_H
#define QAMQPAUTHENTICATOR_H

#include <QString>
#include <QDataStream>

#include "qamqpglobal.h"

class QAMQP_EXPORT QAmqpAuthenticator
{
public:
    virtual ~QAmqpAuthenticator() {}
    virtual QString type() const = 0;
    virtual void write(QDataStream &out) = 0;
};

class QAMQP_EXPORT QAmqpPlainAuthenticator : public QAmqpAuthenticator
{
public:
    QAmqpPlainAuthenticator(const QString &login = QString(), const QString &password = QString());
    virtual ~QAmqpPlainAuthenticator();

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

#endif // QAMQPAUTHENTICATOR_H
