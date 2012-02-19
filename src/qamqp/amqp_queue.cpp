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

void Queue::consume(ConsumeOptions options)
{
	d_func()->consume(options);
}

void Queue::setConsumerTag( const QString &consumerTag )
{
	d_func()->consumerTag = consumerTag;
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
	,  recievingMessage(false)
{

}

QueuePrivate::~QueuePrivate()
{

}


void QueuePrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	ChannelPrivate::_q_method(frame);
	if(frame.channel() != number)
		return;

	if(frame.methodClass() == QAMQP::Frame::fcQueue)
	{
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

	if(frame.methodClass() == QAMQP::Frame::fcBasic)
	{
		switch(frame.id())
		{
		case bmConsumeOk:
			consumeOk(frame);
			break;
		case bmDeliver:
			deliver(frame);
			break;
		default:
			break;
		}
	}


}

void QueuePrivate::declareOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Declared queue: " << name;	
	declared = true;

	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);

	name = readField('s', stream).toString();
	qint32 messageCount = 0, consumerCount = 0;
	stream >> messageCount >> consumerCount;
	qDebug("Message count %d\nConsumer count: %d", messageCount, consumerCount);

	QMetaObject::invokeMethod(q_func(), "declared");
}

void QueuePrivate::deleteOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Deleted or purged queue: " << name;	
	declared = false;

	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	qint32 messageCount = 0;
	stream >> messageCount;
	qDebug("Message count %d", messageCount);
	QMetaObject::invokeMethod(q_func(), "removed");
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

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miDelete);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);

	out << qint16(0); //reserver 1
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

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miPurge);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	out << qint8(0); // no-wait
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



void QueuePrivate::consume( Queue::ConsumeOptions options )
{
	if(!opened)
	{
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, bmConsume);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	writeField('s', out, consumerTag);
	out << qint8(options); // no-wait
	writeField('F', out, TableField());

	frame.setArguments(arguments_);
	sendFrame(frame);
}


void QueuePrivate::consumeOk( const QAMQP::Frame::Method & frame )
{
	qDebug() << "Consume ok: " << name;	
	declared = false;

	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	consumerTag = readField('s',stream).toString();
	qDebug("Consumer tag = %s", qPrintable(consumerTag));
}


void QueuePrivate::deliver( const QAMQP::Frame::Method & frame )
{
	qDebug() << "* Receive message: ";	
	declared = false;

	QByteArray data = frame.arguments();
	QDataStream in(&data, QIODevice::ReadOnly);
	QString consumer_ = readField('s',in).toString();
	if(consumer_ != consumerTag)
	{
		return;
	}

	qlonglong deliveryTag = readField('L',in).toLongLong();
	bool redelivered = readField('t',in).toBool();
	QString exchangeName = readField('s',in).toString();
	QString routingKey = readField('s',in).toString();

	qDebug() << "| Delivery-tag: " << deliveryTag;
	qDebug() << "| Redelivered: " << redelivered;
	qDebug("| Exchange-name: %s", qPrintable(exchangeName));
	qDebug("| Routing-key: %s", qPrintable(routingKey));
}

void QueuePrivate::_q_content( const QAMQP::Frame::Content & frame )
{
	if(frame.channel() != number)
		return;
	qDebug() << "Content-type: " << qPrintable(frame.property(Content::cpContentType).toString());
	qDebug() << "Encoding-type: " << qPrintable(frame.property(Content::cpContentEncoding).toString());
}

void QueuePrivate::_q_body( int channeNumber, const QByteArray & body )
{

}