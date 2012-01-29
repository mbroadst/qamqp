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

QAMQP::Channel::Channel( Client * parent /*= 0*/ )
	: QObject(*new ChannelPrivate, 0)
{
	QT_TRY {
		d_func()->init(parent);
	} QT_CATCH(...) {
		ChannelExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

QAMQP::Channel::Channel( ChannelPrivate &dd, Client* parent )
	: QObject(dd, 0)
{

}
QAMQP::Channel::~Channel()
{
	QObjectPrivate::clearGuards(this);

	QT_TRY {
		QEvent e(QEvent::Destroy);
		QCoreApplication::sendEvent(this, &e);
	} QT_CATCH(const std::exception&) {
		// if this fails we can't do anything about it but at least we are not allowed to throw.
	}
}

void QAMQP::Channel::closeChannel()
{

}

void QAMQP::Channel::reopen()
{

}

QString QAMQP::Channel::name()
{
	return d_func()->name;
}

int QAMQP::Channel::channelNumber()
{
	return d_func()->number;
}

void QAMQP::Channel::setParam( int param )
{

}

void QAMQP::Channel::setName( const QString &name )
{
	d_func()->name = name;
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
//////////////////////////////////////////////////////////////////////////

ChannelPrivate::ChannelPrivate(int version /* = QObjectPrivateVersion */)
	:QObjectPrivate(version), number(++nextChannelNumber_)
{

}

ChannelPrivate::~ChannelPrivate()
{

}

void ChannelPrivate::init(Client * parent)
{
	q_func()->setParent(parent);
	client_ = parent;
}


void ChannelPrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	if(frame.methodClass() != QAMQP::Frame::fcChannel
		|| frame.channel() != number )
		return;

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
}

void ChannelPrivate::_q_open()
{
	qDebug("Open channel #%d", number);
	open();
}


void ChannelPrivate::sendFrame( const QAMQP::Frame::Base & frame )
{
	client_->d_func()->network_->sendFrame(frame);
}


void ChannelPrivate::open()
{
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

}
void ChannelPrivate::flowOk()
{

}

void ChannelPrivate::flowOk( const QAMQP::Frame::Method & frame )
{

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
	client_->d_func()->network_->sendFrame(frame);
}

void ChannelPrivate::close( const QAMQP::Frame::Method & frame )
{
	q_func()->stateChanged(csClosed);

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
	q_func()->stateChanged(csClosed);
}

void ChannelPrivate::openOk( const QAMQP::Frame::Method & frame )
{
	qDebug(">> OpenOK");
	q_func()->stateChanged(csOpened);
}