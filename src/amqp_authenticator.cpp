#include "amqp_table.h"
#include "amqp_frame_p.h"
#include "amqp_authenticator.h"
using namespace QAMQP;

AMQPlainAuthenticator::AMQPlainAuthenticator(const QString &l, const QString &p)
    : login_(l),
      password_(p)
{
}

AMQPlainAuthenticator::~AMQPlainAuthenticator()
{
}

QString AMQPlainAuthenticator::login() const
{
    return login_;
}

QString AMQPlainAuthenticator::password() const
{
    return password_;
}

QString AMQPlainAuthenticator::type() const
{
    return "AMQPLAIN";
}

void AMQPlainAuthenticator::setLogin(const QString &l)
{
    login_ = l;
}

void AMQPlainAuthenticator::setPassword(const QString &p)
{
    password_ = p;
}

void AMQPlainAuthenticator::write(QDataStream &out)
{
    Frame::writeAmqpField(out, ShortString, type());
    Table response;
    response["LOGIN"] = login_;
    response["PASSWORD"] = password_;
    out << response;
}
