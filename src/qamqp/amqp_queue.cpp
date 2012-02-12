#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_exchange.h"

using namespace QAMQP;
using namespace QAMQP::Frame;

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

namespace QAMQP
{
	struct QueueExceptionCleaner
	{
		/* this cleans up when the constructor throws an exception */
		static inline void cleanup(Queue *that, QueuePrivate *d)
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

Queue::Queue( int channelNumber, Client * parent /*= 0*/ )
: Channel(*new QueuePrivate, 0)
{
	QT_TRY {
		d_func()->init(channelNumber, parent);
	} QT_CATCH(...) {
		QueueExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

Queue::~Queue()
{
	remove();
}

void Queue::onOpen()
{
	Q_D(Queue);
	if(d->deleyedDeclare)
	{
		d->declare();
	}
	if(!d->delayedBindings.isEmpty())
	{
		QMap<QString, QString>::iterator i;
		for(i = d->delayedBindings.begin(); i!= d->delayedBindings.end(); ++i )
		{
			d->bind(i.value(), i.key());
		}
		d->delayedBindings.clear();
	}
}

void Queue::onClose()
{
	d_func()->remove(true, true);
}

Queue::QueueOptions Queue::option() const
{
	return d_func()->options;
}



void Queue::declare()
{
	Q_D(Queue);
	declare(d->name, QueueOptions(Durable | AutoDelete));
}

void Queue::declare( const QString &name, QueueOptions options )
{
	Q_D(Queue);
	setName(name);
	d->options = options;
	d->declare();
}

void Queue::remove( bool ifUnused /*= true*/, bool ifEmpty /*= true*/, bool noWait /*= true*/ )
{
	d_func()->remove(ifUnused, ifEmpty, noWait);
}

void Queue::purge()
{
	d_func()->purge();
}

void Queue::bind( const QString & exchangeName, const QString & key )
{
	d_func()->bind(exchangeName, key);
}

void Queue::bind( Exchange * exchange, const QString & key )
{
	if(exchange)
		d_func()->bind(exchange->name(), key);
}

void Queue::unbind( const QString & exchangeName, const QString & key )
{
	d_func()->unbind(exchangeName, key);
}

void Queue::unbind( Exchange * exchange, const QString & key )
{
	if(exchange)
		d_func()->unbind(exchange->name(), key);
}

void Queue::get()
{

}

void Queue::consume()
{

}

void Queue::setConsumerTag( const QString &consumerTag )
{
	d_func()->setConsumerTag(consumerTag);
}

QString Queue::consumerTag() const
{
	return d_func()->consumerTag;
}

//////////////////////////////////////////////////////////////////////////


QueuePrivate::QueuePrivate()
	:ChannelPrivate()
	,  deleyedDeclare(false)
	,  declared(false)
{

}

QueuePrivate::~QueuePrivate()
{

}


void QueuePrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	ChannelPrivate::_q_method(frame);
	if(frame.methodClass() != QAMQP::Frame::fcQueue
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
	case miBindOk:
		bindOk(frame);
		break;
	case miUnbindOk:
		unbindOk(frame);
		break;
	case miPurgeOk:
		deleteOk(frame);
		break;
	default:
		break;
	}
}

void QueuePrivate::declareOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Declared queue: " << name;
	QMetaObject::invokeMethod(q_func(), "declared");
	declared = true;

	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);

	name = readField('s', stream).toString();
	qint32 messageCount = 0, consumerCount = 0;
	stream >> messageCount >> consumerCount;
	qDebug("Message count %d\nConsumer count: %d", messageCount, consumerCount);
}

void QueuePrivate::deleteOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Deleted or purged queue: " << name;
	QMetaObject::invokeMethod(q_func(), "removed");
	declared = false;

	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	qint32 messageCount = 0;
	stream >> messageCount;
	qDebug("Message count %d", messageCount);
}


void QueuePrivate::bindOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Binded to queue: " << name;
	QMetaObject::invokeMethod(q_func(), "binded", Q_ARG(bool, true));
}

void QueuePrivate::unbindOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Unbinded queue: " << name;
	QMetaObject::invokeMethod(q_func(), "binded", Q_ARG(bool, false));
}

void QueuePrivate::declare()
{
	if(!opened)
	{
		deleyedDeclare = true;
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miDeclare);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	out << qint8(options);
	writeField('F', out, TableField());

	frame.setArguments(arguments_);
	sendFrame(frame);
	deleyedDeclare = false;

}

void QueuePrivate::remove( bool ifUnused /*= true*/, bool ifEmpty /*= true*/, bool noWait /*= true*/ )
{
	if(!declared)
		return;

	QAMQP::Frame::Method frame(QAMQP::Frame::fcExchange, miDelete);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);

	out << qint8(0); //reserver 1
	writeField('s', out, name);

	qint8 flag = 0;

	flag |= (ifUnused ? 0x1 : 0);
	flag |= (ifEmpty ? 0x2 : 0);
	flag |= (noWait ? 0x4 : 0);

	out << flag; 

	frame.setArguments(arguments_);
	sendFrame(frame);
}

void QueuePrivate::purge()
{
	if(!opened)
	{
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miBind);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	out << qint8(1); // no-wait
	frame.setArguments(arguments_);
	sendFrame(frame);

}

void QueuePrivate::bind( const QString & exchangeName, const QString & key )
{
	if(!opened)
	{
		delayedBindings[exchangeName] = key;
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miBind);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	writeField('s', out, exchangeName);
	writeField('s', out, key);
	out << qint8(0); // no-wait
	writeField('F', out, TableField());

	frame.setArguments(arguments_);
	sendFrame(frame);
}

void QueuePrivate::unbind( const QString & exchangeName, const QString & key )
{
	if(!opened)
	{
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miBind);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	writeField('s', out, exchangeName);
	writeField('s', out, key);
	writeField('F', out, TableField());

	frame.setArguments(arguments_);
	sendFrame(frame);

}


void QueuePrivate::setConsumerTag( const QString &consumerTag )
{

}