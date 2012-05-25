#ifndef amqp_network_h__
#define amqp_network_h__

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QPointer>
#include <QBuffer>

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
		
		void disconnect();
		void sendFrame();

		void sendFrame(const QAMQP::Frame::Base & frame);

		bool isSsl() const;
		void setSsl(bool value);

		bool autoReconnect() const;
		void setAutoReconnect(bool value);

		QAbstractSocket::SocketState state() const;

	public slots:
		void connectTo(const QString & host = QString(), quint32 port = 0);
		void error( QAbstractSocket::SocketError socketError );

	signals:		
		void connected();
		void disconnected();
		void method(const QAMQP::Frame::Method & method);
		void content(const QAMQP::Frame::Content & content);
		void body(int channeNumber, const QByteArray & body);

	private slots:		
		void readyRead();
		void sslErrors ( const QList<QSslError> & errors );

		void conectionReady();

	private:
		void initSocket(bool ssl = false);
		QPointer<QTcpSocket> socket_;
		QPointer<QBuffer> buffer_;
		QString lastHost_;
		int lastPort_;
		int offsetBuf;
		int leftSize;
		qint8 lastType_;
		bool autoReconnect_;
		int timeOut_;

		bool connect_;
	};
}
#endif // amqp_network_h__
