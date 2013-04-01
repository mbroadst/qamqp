#include "amqp_channel.h"
#include "amqp_channel_p.h"

#include "amqp.h"
#include "amqp_p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

using namespace QAMQP;

namespace QAMQP
{
	int ChannelPrivate::nextChannelNumber_ = 0;
	struct ChannelExceptionCleaner
	{
		/* this cleans up when the constructor throws an exception */
		static inline void cleanup(Channel *that, ChannelPrivate *d)
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


//////////////////////////////////////////////////////////////////////////

QAMQP::Channel::Channel(int channelNumber /*= -1*/, Client * parent /*= 0*/ )
	: pd_ptr(new ChannelPrivate(this))
{
	QT_TRY {
		pd_func()->init(channelNumber, parent);
	} QT_CATCH(...) {
		ChannelExceptionCleaner::cleanup(this, pd_func());
		QT_RETHROW;
	}
}

QAMQP::Channel::Channel( ChannelPrivate * d )
	: pd_ptr(d)
{

}

QAMQP::Channel::~Channel()
{

	QT_TRY {
		QEvent e(QEvent::Destroy);
		QCoreApplication::sendEvent(this, &e);
	} QT_CATCH(const std::exception&) {
		// if this fails we can't do anything about it but at least we are not allowed to throw.
	}
}

void QAMQP::Channel::closeChannel()
{
	P_D(Channel);
	d->needOpen = true;
	if(d->opened)
		d->close(0, QString(), 0,0);
	
}

void QAMQP::Channel::reopen()
{	
	closeChannel();	
	pd_func()->open();
}

QString QAMQP::Channel::name()
{
	return pd_func()->name;
}

int QAMQP::Channel::channelNumber()
{
	return pd_func()->number;
}

void QAMQP::Channel::setName( const QString &name )
{
	pd_func()->name = name;
}

void QAMQP::Channel::stateChanged( int state )
{
	switch(ChannelPrivate::State(state))
	{
	case ChannelPrivate::csOpened:
		emit opened();
		break;
	case ChannelPrivate::csClosed:
		emit closed();
		break;
	case ChannelPrivate::csIdle:
		emit flowChanged(false);
		break;
	case ChannelPrivate::csRunning:
		emit flowChanged(true);
		break;
	}
}

void QAMQP::Channel::_q_method(const Frame::Method &frame)
{
	pd_func()->_q_method(frame);
}

bool QAMQP::Channel::isOpened() const
{
	return pd_func()->opened;
}

void QAMQP::Channel::onOpen()
{

}

void QAMQP::Channel::onClose()
{

}

void QAMQP::Channel::setQOS( qint32 prefetchSize, quint16 prefetchCount )
{
	pd_func()->setQOS(prefetchSize, prefetchCount);
}
//////////////////////////////////////////////////////////////////////////

ChannelPrivate::ChannelPrivate(Channel * q)
	: number(0)
	, opened(false)
	, needOpen(true)
	, pq_ptr(q)
{

}

ChannelPrivate::~ChannelPrivate()
{

}

void ChannelPrivate::init(int channelNumber, Client * parent)
{
	needOpen = channelNumber == -1 ? true : false;
	number = channelNumber == -1 ? ++nextChannelNumber_ : channelNumber;
	nextChannelNumber_ = qMax(channelNumber, (nextChannelNumber_ + 1));
	pq_func()->setParent(parent);
	client_ = parent;
}


bool ChannelPrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	Q_ASSERT(frame.channel() == number);
	if(frame.channel() != number )
		return true;

	if(frame.methodClass() != QAMQP::Frame::fcChannel)
		return false;

	qDebug("Channel#%d:", number);

	switch(frame.id())
	{
	case miOpenOk:
		openOk(frame);
		break;
	case miFlow:
		flow(frame);
		break;
	case miFlowOk:
		flowOk(frame);
		break;
	case miClose:
		close(frame);
		break;
	case miCloseOk:
		closeOk(frame);
		break;
	}
	return true;
}

void ChannelPrivate::_q_open()
{
	open();
}


void ChannelPrivate::sendFrame( const QAMQP::Frame::Base & frame )
{
	if(client_)
	{
		client_->pd_func()->network_->sendFrame(frame);
	}	
}


void ChannelPrivate::open()
{
	if(!needOpen || opened)
		return;

	if(!client_->pd_func()->connection_->isConnected())
		return;
	qDebug("Open channel #%d", number);
	QAMQP::Frame::Method frame(QAMQP::Frame::fcChannel, miOpen);
	frame.setChannel(number);
	QByteArray arguments_;
	arguments_.resize(1);
	arguments_[0] = 0;
	frame.setArguments(arguments_);
	sendFrame(frame);
}


void ChannelPrivate::flow()
{

}

void ChannelPrivate::flow( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);
}

void ChannelPrivate::flowOk()
{

}

void ChannelPrivate::flowOk( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);
}

void ChannelPrivate::close(int code, const QString & text, int classId, int methodId)
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcChannel, miClose);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	QAMQP::Frame::writeField('s',stream, client_->virtualHost());

	stream << qint16(code);
	QAMQP::Frame::writeField('s', stream, text);
	stream << qint16(classId);
	stream << qint16(methodId);

	frame.setArguments(arguments_);
	client_->pd_func()->network_->sendFrame(frame);
}

void ChannelPrivate::close( const QAMQP::Frame::Method & frame )
{
	pq_func()->stateChanged(csClosed);

	qDebug(">> CLOSE");
	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	qint16 code_ = 0, classId, methodId;
	stream >> code_;
	QString text(QAMQP::Frame::readField('s', stream).toString());
	stream >> classId;
	stream >> methodId;

	qDebug(">> code: %d", code_);
	qDebug(">> text: %s", qPrintable(text));
	qDebug(">> class-id: %d", classId);
	qDebug(">> method-id: %d", methodId);

}

void ChannelPrivate::closeOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcChannel, miCloseOk);
	sendFrame(frame);
}

void ChannelPrivate::closeOk( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);
	P_Q(Channel);
	q->stateChanged(csClosed);
	q->onClose();
	opened = false;
}

void ChannelPrivate::openOk( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);
	P_Q(Channel);
	qDebug(">> OpenOK");
	opened = true;
	q->stateChanged(csOpened);
	q->onOpen();
	
}

void ChannelPrivate::setQOS( qint32 prefetchSize, quint16 prefetchCount )
{
	client_->pd_func()->connection_->pd_func()->setQOS(prefetchSize, prefetchCount, number, false);
}


void ChannelPrivate::_q_disconnected()
{
	nextChannelNumber_ = 0;
	opened = false;
}
