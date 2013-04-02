#include "amqp_connection.h"
#include "amqp_connection_p.h"
#include "amqp.h"
#include "amqp_p.h"
#include "amqp_frame.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QVariant>
#include <QTimer>

using namespace QAMQP;

namespace QAMQP
{
	struct ConnectionExceptionCleaner
	{
		/* this cleans up when the constructor throws an exception */
		static inline void cleanup(Connection *that, ConnectionPrivate *d)
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


ConnectionPrivate::ConnectionPrivate( Connection * q)
	: closed_(false)
	, connected(false)
	, pq_ptr(q)
{

}

ConnectionPrivate::~ConnectionPrivate()
{

}

void ConnectionPrivate::init(Client * parent)
{
	pq_func()->setParent(parent);
	client_ = parent;
	heartbeatTimer_ = new QTimer(parent);
	QObject::connect(heartbeatTimer_, SIGNAL(timeout()), 
		pq_func(), SLOT(_q_heartbeat()));
}

void ConnectionPrivate::startOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miStartOk);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);
	
	QAMQP::Frame::TableField clientProperties;
	clientProperties["version"] = QString(QAMQP_VERSION);
	clientProperties["platform"] = QString("Qt %1").arg(qVersion());
	clientProperties["product"] = QString("QAMQP");
	clientProperties.unite(customProperty);
	QAMQP::Frame::serialize(stream, clientProperties);

	client_->pd_func()->auth_->write(stream);

	QAMQP::Frame::writeField('s', stream, "en_US");

	frame.setArguments(arguments_);

	client_->pd_func()->network_->sendFrame(frame);	
}

void ConnectionPrivate::secureOk()
{


}

void ConnectionPrivate::tuneOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miTuneOk);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	stream << qint16(0); //channel_max
	stream << qint32(FRAME_MAX); //frame_max
	stream << qint16(heartbeatTimer_->interval() / 1000); //heartbeat

	frame.setArguments(arguments_);
	client_->pd_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::open()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miOpen);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	QAMQP::Frame::writeField('s',stream, client_->virtualHost());

	stream << qint8(0);
	stream << qint8(0);

	frame.setArguments(arguments_);
	client_->pd_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::start( const QAMQP::Frame::Method & frame )
{
	qDebug(">> Start");
	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	quint8 version_major = 0;
	quint8 version_minor = 0;
	
	stream >> version_major >> version_minor;

	QAMQP::Frame::TableField table;
	QAMQP::Frame::deserialize(stream, table);

	QString mechanisms = QAMQP::Frame::readField('S', stream).toString();
	QString locales = QAMQP::Frame::readField('S', stream).toString();
	
	qDebug(">> version_major: %d", version_major);
	qDebug(">> version_minor: %d", version_minor);

	QAMQP::Frame::print(table);

	qDebug(">> mechanisms: %s", qPrintable(mechanisms));
	qDebug(">> locales: %s", qPrintable(locales));

	startOk();
}

void ConnectionPrivate::secure( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);	
}

void ConnectionPrivate::tune( const QAMQP::Frame::Method & frame )
{
	qDebug(">> Tune");
	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);

	qint16 channel_max = 0,
		heartbeat = 0;
	qint32 frame_max = 0;

	stream >> channel_max;
	stream >> frame_max;
	stream >> heartbeat;

	qDebug(">> channel_max: %d", channel_max);
	qDebug(">> frame_max: %d", frame_max);
	qDebug(">> heartbeat: %d", heartbeat);

	if(heartbeatTimer_)
	{
		heartbeatTimer_->setInterval(heartbeat * 1000);
		if(heartbeatTimer_->interval())
		{
			heartbeatTimer_->start();
		} else {
			heartbeatTimer_->stop();
		}
	}
	tuneOk();
	open();
}

void ConnectionPrivate::openOk( const QAMQP::Frame::Method & frame )
{
	Q_UNUSED(frame);
	qDebug(">> OpenOK");
	connected = true;
	pq_func()->openOk();
}

void ConnectionPrivate::close( const QAMQP::Frame::Method & frame )
{
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
	connected = false;
	client_->pd_func()->network_->error(QAbstractSocket::RemoteHostClosedError);
	QMetaObject::invokeMethod(pq_func(), "disconnected");
}

void ConnectionPrivate::close(int code, const QString & text, int classId, int methodId)
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miClose);
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

void ConnectionPrivate::closeOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miCloseOk);	
	connected = false;
	client_->pd_func()->network_->sendFrame(frame);
	
}

void ConnectionPrivate::closeOk( const QAMQP::Frame::Method & )
{
	connected = false;
	QMetaObject::invokeMethod(pq_func(), "disconnected");
	if(heartbeatTimer_)
	{
		heartbeatTimer_->stop();
	}
}


void ConnectionPrivate::setQOS( qint32 prefetchSize, quint16 prefetchCount, int channel, bool global )
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcBasic, 10);
	frame.setChannel(channel);
	QByteArray arguments_;
	QDataStream out(&arguments_, QIODevice::WriteOnly);

	out << prefetchSize;
	out << prefetchCount;
	out << qint8(global ? 1 : 0);

	frame.setArguments(arguments_);
	client_->pd_func()->network_->sendFrame(frame);	
}

bool ConnectionPrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	Q_ASSERT(frame.methodClass() == QAMQP::Frame::fcConnection);
	if(frame.methodClass() != QAMQP::Frame::fcConnection)
		return true;

	qDebug() << "Connection:";
	
	if (closed_)
	{
		if( frame.id() == miCloseOk)
			closeOk(frame);
		return true;
	}

	switch(MethodId(frame.id()))
	{
		case miStart:
			start(frame);
			break;
		case miSecure:
			secure(frame);
			break;
		case miTune:
			tune(frame);
			break;
		case miOpenOk:
			openOk(frame);
			break;
		case miClose:
			close(frame);
			break;
		case miCloseOk:
			closeOk(frame);
			break;
		default:
			qWarning("Unknown method-id %d", frame.id());
			return false;
	}
	return true;
}

void ConnectionPrivate::_q_heartbeat()
{
	QAMQP::Frame::Heartbeat frame;	
	client_->pd_func()->network_->sendFrame(frame);
}

//////////////////////////////////////////////////////////////////////////

Connection::Connection( Client * parent /*= 0*/ )
	: pd_ptr(new ConnectionPrivate(this))
{
	QT_TRY {
		pd_func()->init(parent);
	} QT_CATCH(...) {
		ConnectionExceptionCleaner::cleanup(this, pd_func());
		QT_RETHROW;
	}
}

Connection::~Connection()
{
	QT_TRY {
		QEvent e(QEvent::Destroy);
		QCoreApplication::sendEvent(this, &e);
	} QT_CATCH(const std::exception&) {
		// if this fails we can't do anything about it but at least we are not allowed to throw.
	}
}

void Connection::startOk()
{
	pd_func()->startOk();
}

void Connection::secureOk()
{
	pd_func()->secureOk();
}

void Connection::tuneOk()
{
	pd_func()->tuneOk();
}

void Connection::open()
{
	pd_func()->open();
}

void Connection::close(int code, const QString & text, int classId , int methodId)
{
	pd_func()->close(code, text, classId, methodId);
}

void Connection::closeOk()
{
	pd_func()->closeOk();
	emit disconnect();
}

void Connection::openOk()
{
	emit connected();
}

void Connection::_q_method(const QAMQP::Frame::Method & frame)
{
	pd_func()->_q_method(frame);
}

bool Connection::isConnected() const
{
	return pd_func()->connected;
}


void Connection::setQOS( qint32 prefetchSize, quint16 prefetchCount )
{
	pd_func()->setQOS(prefetchSize, prefetchCount, 0, true);
}


void Connection::addCustomProperty( const QString & name, const QString & value )
{
	pd_func()->customProperty[name] = value;
}

QString Connection::customProperty( const QString & name ) const
{
	if(pd_func()->customProperty.contains(name))
	{
		return pd_func()->customProperty.value(name).toString();
	}
	return QString();	
}
