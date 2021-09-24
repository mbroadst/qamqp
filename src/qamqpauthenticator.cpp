#include "qamqptable.h"
#include "qamqpframe_p.h"
#include "qamqpauthenticator.h"

QAmqpPlainAuthenticator::QAmqpPlainAuthenticator(const QString &l, const QString &p)
    : login_(l),
      password_(p)
{
}

QAmqpPlainAuthenticator::~QAmqpPlainAuthenticator()
{
}

QString QAmqpPlainAuthenticator::login() const
{
    return login_;
}

QString QAmqpPlainAuthenticator::password() const
{
    return password_;
}

QString QAmqpPlainAuthenticator::type() const
{
    return "AMQPLAIN";
}

void QAmqpPlainAuthenticator::setLogin(const QString &l)
{
    login_ = l;
}

void QAmqpPlainAuthenticator::setPassword(const QString &p)
{
    password_ = p;
}

void QAmqpPlainAuthenticator::write(QDataStream &out)
{
    QAmqpFrame::writeAmqpField(out, QAmqpMetaType::ShortString, type());
    QAmqpTable response;
    response.insert("LOGIN", login_);
    response.insert("PASSWORD", password_);
    out << response;
}
