#include "amqp_exchange.h"
#include "amqp_exchange_p.h"
#include "amqp_queue.h"

using namespace QAMQP;
using namespace QAMQP::Frame;

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>


namespace QAMQP
{
	struct ExchangeExceptionCleaner
	{
		/* this cleans up when the constructor throws an exception */
		static inline void cleanup(Exchange *that, ExchangePrivate *d)
		{
#ifdef QT_NO_EXCEPTIONS
			Q_UNUSED(that);
			Q_UNUSED(d);
#else
			Q_UNUSED(that);
			Q_UNUSED(d);	
#endif
		}
	};	
}

Exchange::Exchange(int channelNumber, Client * parent /*= 0*/ )
: Channel(*new ExchangePrivate, 0)
{
	QT_TRY {
		d_func()->init(channelNumber, parent);
	} QT_CATCH(...) {
		ExchangeExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

Exchange::~Exchange()
{
	remove();
}

void Exchange::onOpen()
{
	Q_D(Exchange);
	if(d->deleyedDeclare)
	{
		d->declare();
	}
}

void Exchange::onClose()
{
	d_func()->remove(true, true);
}

Exchange::ExchangeOptions Exchange::option() const
{
	return d_func()->options;
}

QString Exchange::type() const
{
	return d_func()->type;
}


void Exchange::declare(const QString &type, ExchangeOptions option ,  const TableField & arg)
{
	Q_D(Exchange);
	d->options = option;	
	d->type = type;
	d->arguments = arg;
	d->declare();
}

void Exchange::remove( bool ifUnused /*= true*/, bool noWait /*= true*/ )
{
	d_func()->remove(ifUnused, noWait);
}


void Exchange::bind( QAMQP::Queue * queue )
{
	queue->bind(this, d_func()->name);
}

void Exchange::bind( const QString & queueName )
{

}

void Exchange::bind( const QString & queueName, const QString &key )
{

}

void Exchange::publish( const QString & message, const QString & key )
{
	d_func()->publish(message.toUtf8(), key);
}

//////////////////////////////////////////////////////////////////////////


ExchangePrivate::ExchangePrivate()
	:ChannelPrivate()
	,  deleyedDeclare(false)
	,  declared(false)
{
}


ExchangePrivate::~ExchangePrivate()
{

}


void ExchangePrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	ChannelPrivate::_q_method(frame);
	if(frame.methodClass() != QAMQP::Frame::fcExchange
		|| frame.channel() != number )
		return;

	switch(frame.id())
	{
	case miDeclareOk:
		declareOk(frame);
		break;
	case miDelete:
		deleteOk(frame);
		break;
	default:
		break;
	}
}

void ExchangePrivate::declareOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Declared exchange: " << name;	
	declared = true;
	QMetaObject::invokeMethod(q_func(), "declared");
}

void ExchangePrivate::deleteOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Deleted exchange: " << name;	
	declared = false;
	QMetaObject::invokeMethod(q_func(), "removed");
}

void ExchangePrivate::declare( )
{
	if(!opened)
	{
		deleyedDeclare = true;
		return;
	}

	if(name.isEmpty())
		return;

	QAMQP::Frame::Method frame(QAMQP::Frame::fcExchange, miDeclare);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	stream << qint16(0); //reserver 1
	writeField('s', stream, name);
	writeField('s', stream, type);
	stream << qint8(options);
	writeField('F', stream, ExchangePrivate::arguments);

	frame.setArguments(arguments_);
	sendFrame(frame);
	deleyedDeclare = false;
}

void ExchangePrivate::remove( bool ifUnused /*= true*/, bool noWait /*= true*/ )
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcExchange, miDelete);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	stream << qint16(0); //reserver 1
	writeField('s', stream, name);

	qint8 flag = 0;

	flag |= (ifUnused ? 0x1 : 0);
	flag |= (noWait ? 0x2 : 0);

	stream << flag; //reserver 1

	frame.setArguments(arguments_);
	sendFrame(frame);
}

void ExchangePrivate::publish( const QByteArray & message, const QString & key, const QString &mimeType /*= QString::fromLatin1("text/plain")*/ )
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, bmPublish);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);

	out << qint16(0); //reserver 1
	writeField('s', out, name);
	writeField('s', out, key);
	out << qint8(0); 

	frame.setArguments(arguments_);
	sendFrame(frame);	

	
	QAMQP::Frame::Content content(QAMQP::Frame::fcBasic);
	content.setChannel(number);
	content.setProperty(Content::cpContentType, "text/plain");
	content.setProperty(Content::cpContentEncoding, "utf-8");
	content.setProperty(Content::cpMessageId, "0");
	content.setBody(message);
	sendFrame(content);
	

	QAMQP::Frame::ContentBody body;
	body.setChannel(number);
	body.setBody(message);
	sendFrame(body);
}