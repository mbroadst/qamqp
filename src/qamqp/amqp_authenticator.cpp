#include "amqp_authenticator.h"
#include "amqp_frame.h"

QString QAMQP::AMQPlainAuthenticator::login() const
{
	return login_;
}

QString QAMQP::AMQPlainAuthenticator::password() const
{
	return password_;
}

QAMQP::AMQPlainAuthenticator::AMQPlainAuthenticator( const QString & l /*= QString()*/, const QString & p /*= QString()*/ )
{
	login_ = l;
	password_ = p;
}

QAMQP::AMQPlainAuthenticator::~AMQPlainAuthenticator()
{

}

QString QAMQP::AMQPlainAuthenticator::type() const
{
	return "AMQPLAIN";
}

void QAMQP::AMQPlainAuthenticator::setLogin( const QString& l )
{
	login_ = l;
}

void QAMQP::AMQPlainAuthenticator::setPassword( const QString &p )
{
	password_ = p;
}

void QAMQP::AMQPlainAuthenticator::write( QDataStream & out )
{
	QAMQP::Frame::writeField('s', out, type());
	QAMQP::Frame::TableField response;
	response["LOGIN"] = login_;
	response["PASSWORD"] = password_;
	QAMQP::Frame::serialize(out, response);	
}