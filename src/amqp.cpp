#include "amqp.h"
#include "amqp_p.h"

#include <QTextStream>
#include <QCoreApplication>
#include "qamqp_global.h"
#include "amqp_exchange.h"

using namespace QAMQP;

struct QAMQP::ClientExceptionCleaner
{
	/* this cleans up when the constructor throws an exception */
	static inline void cleanup(Client *that, ClientPrivate *d)
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

//////////////////////////////////////////////////////////////////////////

ClientPrivate::ClientPrivate(int version )
	:QObjectPrivate(version)
	 , port(AMQPPORT)
	 , host(QString::fromLatin1(AMQPHOST))
	 , virtualHost(QString::fromLatin1(AMQPVHOST))
	 , user(QString::fromLatin1(AMQPLOGIN))
	 , password(QString::fromLatin1(AMQPPSWD))
{
	
}


ClientPrivate::~ClientPrivate()
{

}

void ClientPrivate::init(QObject * parent)
{
	q_func()->setParent(parent);
	network_ = new QAMQP::Network(q_func());
	connection_ = new QAMQP::Connection(q_func());

	QObject::connect(network_, SIGNAL(method(const QAMQP::Frame::Method &)),
		connection_, SLOT(_q_method(const QAMQP::Frame::Method &)));

	ClientPrivate::connect();
}

void ClientPrivate::init(QObject * parent, const QUrl & con)
{
	Q_Q(QAMQP::Client);
	if(con.scheme() == AMQPSCHEME )
	{
		q->setPassword(con.password());
		q->setUser(con.userName());
		q->setPort(con.port());
		q->setHost(con.host());				
		q->setVirtualHost(con.path());
	}
	init(parent);
}

void ClientPrivate::printConnect() const
{
	QTextStream stream(stdout);
	stream <<  "port  = " << port << endl;
	stream <<  "host  = " << host << endl;
	stream <<  "vhost = " << virtualHost << endl;
	stream <<  "user  = " << user << endl;
	stream <<  "passw = " << password << endl;
}

void ClientPrivate::connect()
{
	ClientPrivate::sockConnect();
	ClientPrivate::login();
}

void ClientPrivate::parseCnnString( const QUrl & connectionString )
{

}

void ClientPrivate::sockConnect()
{
	network_->connectTo(host, port);
}

void ClientPrivate::login()
{

}

Exchange * ClientPrivate::createExchange( const QString &name )
{
	Exchange * exchange_ = new Exchange(q_func());
	QObject::connect(network_, SIGNAL(method(const QAMQP::Frame::Method &)),
		exchange_, SLOT(_q_method(const QAMQP::Frame::Method &)));

	QObject::connect(connection_, SIGNAL(connected()), exchange_, SLOT(_q_open()));
	return exchange_;
}

Queue * ClientPrivate::createQueue( const QString &name )
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////


QAMQP::Client::Client( QObject * parent /*= 0*/ )
	: QObject(*new ClientPrivate, 0)
{
	QT_TRY {
		d_func()->init(parent);
	} QT_CATCH(...) {
		ClientExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

QAMQP::Client::Client( const QUrl & connectionString, QObject * parent /*= 0*/ )
	: QObject(*new ClientPrivate, 0)
{
	QT_TRY {
		d_func()->init(parent, connectionString);
	} QT_CATCH(...) {
		ClientExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

QAMQP::Client::Client(ClientPrivate &dd, QObject* parent, const QUrl & connectionString)
	: QObject(dd, 0)
{
	Q_D(QAMQP::Client);
	QT_TRY {
		d->init(parent, connectionString);
	} QT_CATCH(...) {
		ClientExceptionCleaner::cleanup(this, d_func());
		QT_RETHROW;
	}
}

QAMQP::Client::~Client()
{
	QObjectPrivate::clearGuards(this);
	QT_TRY {
		QEvent e(QEvent::Destroy);
		QCoreApplication::sendEvent(this, &e);
	} QT_CATCH(const std::exception&) {
		// if this fails we can't do anything about it but at least we are not allowed to throw.
	}
}

quint32 QAMQP::Client::port() const
{
	return d_func()->port;
}

void QAMQP::Client::setPort( quint32 port )
{
	d_func()->port = port;
}

QString QAMQP::Client::host() const
{
	return d_func()->host;
}

void QAMQP::Client::setHost( const QString & host )
{
	d_func()->host = host;
}

QString QAMQP::Client::virtualHost() const
{
	return d_func()->virtualHost;
}

void QAMQP::Client::setVirtualHost( const QString & virtualHost )
{
	d_func()->virtualHost = virtualHost;
}

QString QAMQP::Client::user() const
{
	return d_func()->user;
}

void QAMQP::Client::setUser( const QString & user )
{
	d_func()->user = user;
}

QString QAMQP::Client::password() const
{
	return d_func()->password;
}

void QAMQP::Client::setPassword( const QString & password )
{
	d_func()->password = password;
}

void QAMQP::Client::printConnect() const
{
#ifdef _DEBUG
	d_func()->printConnect();
#endif // _DEBUG
}

void QAMQP::Client::closeChannel()
{

}

Exchange * QAMQP::Client::createExchange()
{
	return d_func()->createExchange(QString());
}

Exchange * QAMQP::Client::createExchange( const QString &name )
{
	return d_func()->createExchange(name);
}

Queue * QAMQP::Client::createQueue()
{
	return d_func()->createQueue(QString());
}

Queue * QAMQP::Client::createQueue( const QString &name )
{
	return d_func()->createQueue(name);
}