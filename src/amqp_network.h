#ifndef amqp_network_h__
#define amqp_network_h__

#include <QObject>
#include <QTcpSocket>
#include <QPointer>
#include <QBuffer>
#include <QQueue>

#include "amqp_frame.h"

namespace QAMQP
{
	class Network : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(Network)
	public:
		Network(QObject * parent = 0);
		~Network();

		void connectTo(const QString & host, quint32 port);
		void disconnect();
		void sendFrame();

		void sendFrame(const QAMQP::Frame::BasePtr &frame);

	signals:
		void method(const QAMQP::Frame::Method & method);

	private slots:
		void connected();
		void disconnected();
		void error( QAbstractSocket::SocketError socketError );
		void readyRead();

	private:
		QPointer<QTcpSocket> socket_;
		QPointer<QBuffer> buffer_;
		QQueue<QAMQP::Frame::BasePtr> outFrames_;
		int offsetBuf;
		int leftSize;
		qint8 lastType_;
	};
}
#endif // amqp_network_h__
