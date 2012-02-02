#include "amqp_connection.h"
#include "amqp_connection_p.h"
#include "amqp.h"
#include "amqp_p.h"
#include "amqp_frame.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

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


ConnectionPrivate::ConnectionPrivate( int version /*= QObjectPrivateVersion*/ )
	:QObjectPrivate(version), closed_(false)
{

}

ConnectionPrivate::~ConnectionPrivate()
{

}

void ConnectionPrivate::init(Client * parent)
{
	q_func()->setParent(parent);
	client_ = parent;
}

void ConnectionPrivate::startOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miStartOk);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);
	
	QAMQP::Frame::TableField clientProperties;
	clientProperties["version"] = "0.0.1";
	clientProperties["platform"] = QString("Qt %1").arg(qVersion());
	clientProperties["product"] = "QAMQP";
	clientProperties["site"] = "http://vmp.ru";
	QAMQP::Frame::serialize(stream, clientProperties);

	QAMQP::Frame::writeField(QAMQP::Frame::fkShortString, stream, "AMQPLAIN");
	QAMQP::Frame::TableField response;
	response["LOGIN"] = client_->user();
	response["PASSWORD"] = client_->password();
	QAMQP::Frame::serialize(stream, response);
	QAMQP::Frame::writeField(QAMQP::Frame::fkShortString, stream, "en_US");

	frame.setArguments(arguments_);

	client_->d_func()->network_->sendFrame(frame);
	
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
	stream << qint16(0); //heartbeat

	frame.setArguments(arguments_);
	client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::open()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miOpen);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	QAMQP::Frame::writeField(QAMQP::Frame::fkShortString,stream, client_->virtualHost());

	stream << qint8(0);
	stream << qint8(0);

	frame.setArguments(arguments_);
	client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::start( const QAMQP::Frame::Method & frame )
{
	qDebug(">> Start");
	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	quint8 version_major = 0;
	quint8 version_minor = 0;
	
	stream >> version_major;
	stream >> version_minor;

	QAMQP::Frame::TableField table;
	QAMQP::Frame::deserialize(stream, table);

	QString mechanisms = QAMQP::Frame::readField(QAMQP::Frame::fkLongString, stream).toString();
	QString locales = QAMQP::Frame::readField(QAMQP::Frame::fkLongString, stream).toString();
	
	qDebug(">> version_major: %d", version_major);
	qDebug(">> version_minor: %d", version_minor);

	QAMQP::Frame::print(table);

	qDebug(">> mechanisms: %s", qPrintable(mechanisms));
	qDebug(">> locales: %s", qPrintable(locales));

	startOk();
}

void ConnectionPrivate::secure( const QAMQP::Frame::Method & frame )
{
	
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

	tuneOk();
	open();
}

void ConnectionPrivate::openOk( const QAMQP::Frame::Method & frame )
{
	qDebug(">> OpenOK");
	q_func()->openOk();
}

void ConnectionPrivate::close( const QAMQP::Frame::Method & frame )
{
	qDebug(">> CLOSE");
	QByteArray data = frame.arguments();
	QDataStream stream(&data, QIODevice::ReadOnly);
	qint16 code_ = 0, classId, methodId;
	stream >> code_;
	QString text(QAMQP::Frame::readField(QAMQP::Frame::fkShortString, stream).toString());
	stream >> classId;
	stream >> methodId;

	qDebug(">> code: %d", code_);
	qDebug(">> text: %s", qPrintable(text));
	qDebug(">> class-id: %d", classId);
	qDebug(">> method-id: %d", methodId);
}

void ConnectionPrivate::close(int code, const QString & text, int classId, int methodId)
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miClose);
	QByteArray arguments_;
	QDataStream stream(&arguments_, QIODevice::WriteOnly);

	QAMQP::Frame::writeField(QAMQP::Frame::fkShortString,stream, client_->virtualHost());

	stream << qint16(code);
	QAMQP::Frame::writeField(QAMQP::Frame::fkShortString, stream, text);
	stream << qint16(classId);
	stream << qint16(methodId);

	frame.setArguments(arguments_);
	client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::closeOk()
{
	QAMQP::Frame::Method frame(QAMQP::Frame::fcConnection, miCloseOk);
	client_->d_func()->network_->sendFrame(frame);
}

void ConnectionPrivate::closeOk( const QAMQP::Frame::Method & )
{
	QMetaObject::invokeMethod(q_func(), "disconnected");
}

void ConnectionPrivate::_q_method( const QAMQP::Frame::Method & frame )
{
	if(frame.methodClass() != QAMQP::Frame::fcConnection)
		return;

	qDebug() << "Connection:";
	
	if (closed_)
	{
		if( frame.id() == miCloseOk)
			closeOk(frame);
		return;
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
	}

}

//////////////////////////////////////////////////////////////////////////

Connection::Connection( Client * parent /*= 0*/ )
	: QObject(*new ConnectionPrivate, 0)
{
	QT_TRY {
		d_func()->init(parent);
	} QT_CATCH(...) {
		ConnectionExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

Connection::Connection(ConnectionPrivate &dd, Client* parent)
: QObject(dd, 0)
{
	Q_D(QAMQP::Connection);
	QT_TRY {
		d->init(parent);
	} QT_CATCH(...) {
		ConnectionExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

Connection::~Connection()
{
	QObjectPrivate::clearGuards(this);

	QT_TRY {
		QEvent e(QEvent::Destroy);
		QCoreApplication::sendEvent(this, &e);
	} QT_CATCH(const std::exception&) {
		// if this fails we can't do anything about it but at least we are not allowed to throw.
	}
}

void Connection::startOk()
{
	d_func()->startOk();
}

void Connection::secureOk()
{
	d_func()->secureOk();
}

void Connection::tuneOk()
{
	d_func()->tuneOk();
}

void Connection::open()
{
	d_func()->open();
}

void Connection::close(int code, const QString & text, int classId , int methodId)
{
	d_func()->close(code, text, classId, methodId);
}

void Connection::closeOk()
{
	d_func()->closeOk();
	emit disconnect();
}

void Connection::openOk()
{
	emit connected();
}