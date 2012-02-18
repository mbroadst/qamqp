#include "amqp_network.h"

#include <QDebug>

QAMQP::Network::Network( QObject * parent /*= 0*/ )
{
	qRegisterMetaType<QAMQP::Frame::Method>("QAMQP::Frame::Method");

	socket_ = new QTcpSocket(this);
	buffer_ = new QBuffer(this);
	offsetBuf = 0;
	leftSize = 0;


	buffer_->open(QIODevice::ReadWrite);
	connect(socket_, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket_, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));	
}

QAMQP::Network::~Network()
{
	disconnect();
}

void QAMQP::Network::connectTo( const QString & host, quint32 port )
{
	socket_->connectToHost(host, port);
}

void QAMQP::Network::disconnect()
{
	if(socket_)
		socket_->abort();
}

void QAMQP::Network::connected()
{
	char header_[8] = {'A', 'M', 'Q', 'P', 0,0,9,1};
	socket_->write(header_, 8);
}

void QAMQP::Network::disconnected()
{

}

void QAMQP::Network::error( QAbstractSocket::SocketError socketError )
{

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
	QDataStream stream(socket_);
	frame.toStream(stream);
}