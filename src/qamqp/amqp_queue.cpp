#include "amqp_queue.h"
#include "amqp_queue_p.h"
#include "amqp_exchange.h"

using namespace QAMQP;
using namespace QAMQP::Frame;

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QFile>

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
: Channel(new QueuePrivate(this))
{
	QT_TRY {
		pd_func()->init(channelNumber, parent);
	} QT_CATCH(...) {
		QueueExceptionCleaner::cleanup(this, pd_func());
		QT_RETHROW;
	}
}

Queue::~Queue()
{
	remove();
}

void Queue::onOpen()
{
	P_D(Queue);
	if(d->delayedDeclare)
	{
		d->declare();
	}
	if(!d->delayedBindings.isEmpty())
	{
		typedef QPair<QString, QString> BindingPair;
		foreach(BindingPair binding, d->delayedBindings)
		{
			d->bind(binding.first, binding.second);
		}
		d->delayedBindings.clear();
	}
}

void Queue::onClose()
{
	pd_func()->remove(true, true);
}

Queue::QueueOptions Queue::option() const
{
	return pd_func()->options;
}

void Queue::setNoAck( bool noAck )
{
	pd_func()->noAck = noAck;
}

bool Queue::noAck() const
{
	return pd_func()->noAck;
}

void Queue::declare()
{
	P_D(Queue);
	declare(d->name, QueueOptions(Durable | AutoDelete));
}

void Queue::declare( const QString &name, QueueOptions options )
{
	P_D(Queue);
	setName(name);
	d->options = options;
	d->declare();
}

void Queue::remove( bool ifUnused /*= true*/, bool ifEmpty /*= true*/, bool noWait /*= true*/ )
{
	pd_func()->remove(ifUnused, ifEmpty, noWait);
}

void Queue::purge()
{
	pd_func()->purge();
}

void Queue::bind( const QString & exchangeName, const QString & key )
{
	pd_func()->bind(exchangeName, key);
}

void Queue::bind( Exchange * exchange, const QString & key )
{
	if(exchange)
		pd_func()->bind(exchange->name(), key);
}

void Queue::unbind( const QString & exchangeName, const QString & key )
{
	pd_func()->unbind(exchangeName, key);
}

void Queue::unbind( Exchange * exchange, const QString & key )
{
	if(exchange)
		pd_func()->unbind(exchange->name(), key);
}

void Queue::_q_content(const Content &frame)
{
	pd_func()->_q_content(frame);
}

void Queue::_q_body(const ContentBody &frame)
{
	pd_func()->_q_body(frame);
}

QAMQP::MessagePtr Queue::getMessage()
{
	return pd_func()->messages_.dequeue();
}

bool Queue::hasMessage() const
{

	if(pd_func()->messages_.isEmpty())
	{
		return false;
	}
	const MessagePtr &q = pd_func()->messages_.head();
	return q->leftSize == 0;
}

void Queue::consume(ConsumeOptions options)
{
	pd_func()->consume(options);
}

void Queue::setConsumerTag( const QString &consumerTag )
{
	pd_func()->consumerTag = consumerTag;
}

QString Queue::consumerTag() const
{
	return pd_func()->consumerTag;
}


void Queue::get()
{
	pd_func()->get();
}


void Queue::ack( const MessagePtr & message )
{
	pd_func()->ack(message);
}

//////////////////////////////////////////////////////////////////////////


QueuePrivate::QueuePrivate(Queue * q)
	:ChannelPrivate(q)
	,  delayedDeclare(false)
	,  declared(false)
	,  noAck(true)
	,  recievingMessage(false)
{

}

QueuePrivate::~QueuePrivate()
{
	
}


bool QueuePrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	if(ChannelPrivate::_q_method(frame))
		return true;

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
		return true;
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
		case bmGetOk:
			getOk(frame);
			break;
		case bmGetEmpty:
			QMetaObject::invokeMethod(pq_func(), "empty");
			break;
		default:
			break;
		}
		return true;
	}



	return false;
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

	QMetaObject::invokeMethod(pq_func(), "declared");
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
	QMetaObject::invokeMethod(pq_func(), "removed");
}


void QueuePrivate::bindOk( const QAMQP::Frame::Method &  )
{
	qDebug() << "Binded to queue: " << name;
	QMetaObject::invokeMethod(pq_func(), "binded", Q_ARG(bool, true));
}

void QueuePrivate::unbindOk( const QAMQP::Frame::Method &  )
{
	qDebug() << "Unbinded queue: " << name;
	QMetaObject::invokeMethod(pq_func(), "binded", Q_ARG(bool, false));
}

void QueuePrivate::declare()
{
	if(!opened)
	{
		delayedDeclare = true;
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
	delayedDeclare = false;
	

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
		delayedBindings.append(QPair<QString,QString>(exchangeName, key));
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

	QAMQP::Frame::Method frame(QAMQP::Frame::fcQueue, miUnbind);
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


void QueuePrivate::get()
{
	if(!opened)
	{
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, bmGet);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << qint16(0); //reserver 1
	writeField('s', out, name);
	out << qint8(noAck ? 1 : 0); // noAck

	frame.setArguments(arguments_);
	sendFrame(frame);
}


void QueuePrivate::getOk( const QAMQP::Frame::Method & frame )
{
	QByteArray data = frame.arguments();
	QDataStream in(&data, QIODevice::ReadOnly);

	qlonglong deliveryTag = readField('L',in).toLongLong();
	bool redelivered = readField('t',in).toBool();
	QString exchangeName = readField('s',in).toString();
	QString routingKey = readField('s',in).toString();

	Q_UNUSED(redelivered)

	MessagePtr newMessage = MessagePtr(new Message);
	newMessage->routeKey = routingKey;
	newMessage->exchangeName = exchangeName;
	newMessage->deliveryTag = deliveryTag;
	messages_.enqueue(newMessage);
}


void QueuePrivate::ack( const MessagePtr & Message )
{
	if(!opened)
	{
		return;
	}

	QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, bmAck);
	frame.setChannel(number);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);
	out << Message->deliveryTag; //reserver 1
	out << qint8(0); // noAck

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

	Q_UNUSED(redelivered)

	MessagePtr newMessage = MessagePtr(new Message);
	newMessage->routeKey = routingKey;
	newMessage->exchangeName = exchangeName;
	newMessage->deliveryTag = deliveryTag;
	messages_.enqueue(newMessage);

}

void QueuePrivate::_q_content( const QAMQP::Frame::Content & frame )
{
	Q_ASSERT(frame.channel() == number);
	if(frame.channel() != number)
		return;
	if(messages_.isEmpty())
	{
		qErrnoWarning("Received content-header without method frame before");
		return;
	}
	MessagePtr &message = messages_.last();
	message->leftSize = frame.bodySize();
	QHash<int, QVariant>::ConstIterator i;
	for (i = frame.properties_.begin(); i != frame.properties_.end(); ++i)
	{
		message->property[Message::MessageProperty(i.key())]= i.value();
	}
}

void QueuePrivate::_q_body(const QAMQP::Frame::ContentBody & frame)
{
	Q_ASSERT(frame.channel() == number);
	if(frame.channel() != number)
		return;

	if(messages_.isEmpty())
	{
		qErrnoWarning("Received content-body without method frame before");
		return;
	}
	MessagePtr &message = messages_.last();
	message->payload.append(frame.body());
	message->leftSize -= frame.body().size();
	
	if(message->leftSize == 0 && messages_.size() == 1)
	{
		emit pq_func()->messageReceived(pq_func());
	}
}
