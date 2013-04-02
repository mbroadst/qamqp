#ifndef amqp_network_h__
#define amqp_network_h__

#include <QObject>
#include <QTcpSocket>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif
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
		typedef qint16 Channel;

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

		void setMethodHandlerConnection(Frame::MethodHandler* pMethodHandlerConnection);
		void addMethodHandlerForChannel(Channel channel, Frame::MethodHandler* pHandler);
		void addContentHandlerForChannel(Channel channel, Frame::ContentHandler* pHandler);
		void addContentBodyHandlerForChannel(Channel channel, Frame::ContentBodyHandler* pHandler);

	public slots:
		void connectTo(const QString & host = QString(), quint32 port = 0);
		void error( QAbstractSocket::SocketError socketError );

	signals:		
		void connected();
		void disconnected();

	private slots:		
		void readyRead();

		void sslErrors ( );


		void conectionReady();

	private:
		void initSocket(bool ssl = false);
		QPointer<QTcpSocket> socket_;
        QByteArray buffer_;
		QString lastHost_;
		int lastPort_;
		bool autoReconnect_;
		int timeOut_;
		bool connect_;

		Frame::MethodHandler* m_pMethodHandlerConnection;

		QHash<Channel, QList<Frame::MethodHandler*> > m_methodHandlersByChannel;
		QHash<Channel, QList<Frame::ContentHandler*> > m_contentHandlerByChannel;
		QHash<Channel, QList<Frame::ContentBodyHandler*> > m_bodyHandlersByChannel;
	};
}
#endif // amqp_network_h__
