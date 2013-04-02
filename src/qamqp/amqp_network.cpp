#include "amqp_network.h"
#include <QDebug>
#include <QTimer>
#include <QtEndian>

QAMQP::Network::Network( QObject * parent /*= 0*/ ):QObject(parent)
{
	qRegisterMetaType<QAMQP::Frame::Method>("QAMQP::Frame::Method");

	buffer_.reserve(Frame::HEADER_SIZE);
	timeOut_ = 1000;
	connect_ = false;

	initSocket(false);
}

QAMQP::Network::~Network()
{
	disconnect();
}

void QAMQP::Network::connectTo( const QString & host, quint32 port )
{
	if(!socket_)
	{
		qWarning("AMQP: Socket didn't create.");
		return;
	}
	QString h(host);
	int p(port);
	connect_ = true;
	if(host.isEmpty())
		h = lastHost_ ;
	if(port == 0)
		p = lastPort_;

	if (isSsl())
	{
#ifndef QT_NO_SSL
		static_cast<QSslSocket *>(socket_.data())->connectToHostEncrypted(h, p);
#else
		qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
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
			
		default:
			qWarning() << "AMQP: Socket Error: " << socket_->errorString();
			break;
	}

	if( autoReconnect_ && connect_ )
	{
		QTimer::singleShot(timeOut_, this, SLOT(connectTo()));
	}

}

void QAMQP::Network::readyRead()
{
	while(socket_->bytesAvailable() >= Frame::HEADER_SIZE)
	{
		char* headerData = buffer_.data();
		socket_->peek(headerData, Frame::HEADER_SIZE);
		const quint32 payloadSize = qFromBigEndian<quint32>(*(quint32*)&headerData[3]);
		const qint64 readSize = Frame::HEADER_SIZE+payloadSize+Frame::FRAME_END_SIZE;
		if(socket_->bytesAvailable() >= readSize)
		{
			buffer_.resize(readSize);
			socket_->read(buffer_.data(), readSize);
			const char* bufferData = buffer_.constData();
			const quint8 type = *(quint8*)&bufferData[0];
			const quint8 magic = *(quint8*)&bufferData[Frame::HEADER_SIZE+payloadSize];
			if(magic != QAMQP::Frame::FRAME_END)
			{
				qWarning() << "Wrong end frame";
			}

			QDataStream streamB(&buffer_, QIODevice::ReadOnly);
			switch(QAMQP::Frame::Type(type))
			{
			case QAMQP::Frame::ftMethod:
				{
					QAMQP::Frame::Method frame(streamB);
					if(frame.methodClass() == QAMQP::Frame::fcConnection)
					{
						m_pMethodHandlerConnection->_q_method(frame);
					}
					else
					{
						foreach(Frame::MethodHandler* pMethodHandler, m_methodHandlersByChannel[frame.channel()])
						{
							pMethodHandler->_q_method(frame);
						}
					}
				}
				break;
			case QAMQP::Frame::ftHeader:
				{
					QAMQP::Frame::Content frame(streamB);
					foreach(Frame::ContentHandler* pMethodHandler, m_contentHandlerByChannel[frame.channel()])
					{
						pMethodHandler->_q_content(frame);
					}
				}
				break;
			case QAMQP::Frame::ftBody:
				{
					QAMQP::Frame::ContentBody frame(streamB);
					foreach(Frame::ContentBodyHandler* pMethodHandler, m_bodyHandlersByChannel[frame.channel()])
					{
						pMethodHandler->_q_body(frame);
					}
				}
				break;
			case QAMQP::Frame::ftHeartbeat:
				{
					qDebug("AMQP: Heartbeat");
				}
				break;
			default:
				qWarning() << "AMQP: Unknown frame type: " << type;
			}
		}
		else
		{
			break;
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
	if(socket_)
	{
		return QString(socket_->metaObject()->className()).compare( "QSslSocket", Qt::CaseInsensitive) == 0;	
	}
	return false;
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
#ifndef QT_NO_SSL
		socket_ = new QSslSocket(this);
		QSslSocket * ssl_= static_cast<QSslSocket*> (socket_.data());
		ssl_->setProtocol(QSsl::AnyProtocol);
		connect(socket_, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors()));	

		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
#else
	qWarning("AMQP: You library has builded with QT_NO_SSL option.");
#endif
	} else {
		socket_ = new QTcpSocket(this);		
		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
	}
	
	if(socket_)
	{
		connect(socket_, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
		connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
		connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));	
	}
}


void QAMQP::Network::sslErrors( )
{
	#ifndef QT_NO_SSL
	static_cast<QSslSocket*>(socket_.data())->ignoreSslErrors();
	#endif
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
	if(socket_)
	{
		return socket_->state();
	} else {
		return QAbstractSocket::UnconnectedState;
	}
	
}

void QAMQP::Network::setMethodHandlerConnection(Frame::MethodHandler* pMethodHandlerConnection)
{
	m_pMethodHandlerConnection = pMethodHandlerConnection;
}

void QAMQP::Network::addMethodHandlerForChannel(Channel channel, Frame::MethodHandler* pHandler)
{
	m_methodHandlersByChannel[channel].append(pHandler);
}

void QAMQP::Network::addContentHandlerForChannel(Channel channel, Frame::ContentHandler* pHandler)
{
	m_contentHandlerByChannel[channel].append(pHandler);
}

void QAMQP::Network::addContentBodyHandlerForChannel(Channel channel, Frame::ContentBodyHandler* pHandler)
{
	m_bodyHandlersByChannel[channel].append(pHandler);
}
