#include "amqp_network.h"
#include <QDebug>
#include <QTimer>

QAMQP::Network::Network( QObject * parent /*= 0*/ ):QObject(parent)
{
	qRegisterMetaType<QAMQP::Frame::Method>("QAMQP::Frame::Method");

	
	buffer_ = new QBuffer(this);
	offsetBuf = 0;
	leftSize = 0;
	timeOut_ = 1000;
	connect_ = false;

	buffer_->open(QIODevice::ReadWrite);

	initSocket(false);
}

QAMQP::Network::~Network()
{
	disconnect();
}

void QAMQP::Network::connectTo( const QString & host, quint32 port )
{
	QString h(host);
	int p(port);
	connect_ = true;
	if(host.isEmpty())
		h = lastHost_ ;
	if(port == 0)
		p = lastPort_;

	if (isSsl())
	{
		static_cast<QSslSocket *>(socket_.data())->connectToHostEncrypted(h, p);
	} else {
		socket_->connectToHost(h, p);
	}

	lastHost_ = h;
	lastPort_ = p;	
}

void QAMQP::Network::disconnect()
{
	connect_ = false;
	if(socket_)
		socket_->close();
}

void QAMQP::Network::error( QAbstractSocket::SocketError socketError )
{
	if(timeOut_ == 0)
	{
		timeOut_ = 1000;
	} else {		
		if(timeOut_ < 120000)
		{
			timeOut_ *= 5;
		}
	}

	Q_UNUSED(socketError);
	switch(socketError)
	{
		case QAbstractSocket::ConnectionRefusedError:
		case QAbstractSocket::RemoteHostClosedError:
		case QAbstractSocket::SocketTimeoutError:
		case QAbstractSocket::NetworkError:
		case QAbstractSocket::ProxyConnectionClosedError:
		case QAbstractSocket::ProxyConnectionRefusedError:
		case QAbstractSocket::ProxyConnectionTimeoutError:
			if( autoReconnect_ && connect_ )
			{
				QTimer::singleShot(timeOut_, this, SLOT(connectTo()));
			}
			break;

		default:
			qWarning() << "AMQP Socket Error: " << socket_->errorString();
			break;
	}
}

void QAMQP::Network::readyRead()
{
	QDataStream streamA(socket_);
	QDataStream streamB(buffer_);
	
	/*
	Вычитать заголовок, поместить в буфер
	вычитать весь фрейм, если фрейм вычитан то кинуть на разбор его
	*/
	while(!socket_->atEnd())
	{
		if(leftSize == 0) // Если ранее прочитан был весь фрейм, то читаем заголовок фрейма
		{
			lastType_  = 0;
			qint16 channel_  = 0;
			leftSize  = 0;
			offsetBuf = 0;

			streamA >> lastType_;
			streamB << lastType_;
			streamA >> channel_;
			streamB << channel_;
			streamA >> leftSize;
			streamB << leftSize;
			leftSize++; // увеличим размер на 1, для захвата конца фрейма
		}

		QByteArray data_;
		data_.resize(leftSize);
		offsetBuf = streamA.readRawData(data_.data(), data_.size());
		leftSize -= offsetBuf;
		streamB.writeRawData(data_.data(), offsetBuf);
		if(leftSize == 0)
		{		
			buffer_->reset();
			switch(QAMQP::Frame::Type(lastType_))
			{
			case QAMQP::Frame::ftMethod:
				{
					QAMQP::Frame::Method frame(streamB);
					emit method(frame);
				}
				break;
			case QAMQP::Frame::ftHeader:
				{
					QAMQP::Frame::Content frame(streamB);
					emit content(frame);
				}
				break;
			case QAMQP::Frame::ftBody:
				{
					QAMQP::Frame::ContentBody frame(streamB);
					emit body(frame.channel(), frame.body());
				}
				break;
			default:
				qWarning("Unknown frame type");
			}
			buffer_->reset();
		}
	}
}

void QAMQP::Network::sendFrame( const QAMQP::Frame::Base & frame )
{
	if(socket_->state() == QAbstractSocket::ConnectedState)	
	{
		QDataStream stream(socket_);
		frame.toStream(stream);
	}
}

bool QAMQP::Network::isSsl() const
{
	return QString(socket_->metaObject()->className()).compare( "QSslSocket", Qt::CaseInsensitive) == 0;
}

void QAMQP::Network::setSsl( bool value )
{
	initSocket(value);
}

void QAMQP::Network::initSocket( bool ssl /*= false*/ )
{
	if(socket_)
		delete socket_;

	if(ssl)
	{		
		socket_ = new QSslSocket(this);
		QSslSocket * ssl_= static_cast<QSslSocket*> (socket_.data());
		ssl_->setProtocol(QSsl::AnyProtocol);
		connect(socket_, SIGNAL(sslErrors(const QList<QSslError> &)),
			this, SLOT(sslErrors(const QList<QSslError> &)));	

		//connect(socket_, SIGNAL(encrypted()), this, SLOT(conectionReady()));
		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
	} else {
		socket_ = new QTcpSocket(this);		
		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
	}
	
	connect(socket_, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
	connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));	
}

void QAMQP::Network::sslErrors( const QList<QSslError> & errors )
{
	static_cast<QSslSocket*>(socket_.data())->ignoreSslErrors();
}

void QAMQP::Network::conectionReady()
{
	emit connected();
	timeOut_ = 0;
	char header_[8] = {'A', 'M', 'Q', 'P', 0,0,9,1};
	socket_->write(header_, 8);
}

bool QAMQP::Network::autoReconnect() const
{
	return autoReconnect_;
}

void QAMQP::Network::setAutoReconnect( bool value )
{
	autoReconnect_ = value;
}

QAbstractSocket::SocketState QAMQP::Network::state() const
{
	return socket_->state();
}
