#include "amqp_private.h"

QAMQP::Base::Base( int version /*= QObjectPrivateVersion*/ )
{
	outArguments_ = QSharedPointer<QBuffer>(new QBuffer());
	outArguments_->open(QIODevice::WriteOnly);
}

QAMQP::Base::~Base()
{
	outArguments_.clear();
	inFrame_.clear();
	outFrame_.clear();
}

void QAMQP::Base::startMethod( int id )
{
	outFrame_ = QAMQP::Frame::BasePtr(new QAMQP::Frame::Method(methodClass(), id));
	streamOut_.setDevice(outArguments_.data());

}

void QAMQP::Base::writeArgument( QAMQP::Frame::FieldValueKind type, QVariant value )
{

}

void QAMQP::Base::writeArgument( QVariant value )
{

}

void QAMQP::Base::endWrite()
{

}

void QAMQP::Base::startRead( QAMQP::Frame::BasePtr frame )
{

}

QVariant QAMQP::Base::readArgument( QAMQP::Frame::FieldValueKind type )
{

}

void QAMQP::Base::endRead()
{

}