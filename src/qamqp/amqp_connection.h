#ifndef amqp_connection_h__
#define amqp_connection_h__

#include <QObject>
#include "amqp_frame.h"
#include "amqp_global.h"

namespace QAMQP
{
	class ConnectionPrivate;
	class ChannelPrivate;
	class ClientPrivate;
	class Client;
	class Connection : public QObject, public Frame::MethodHandler
	{
		Q_OBJECT
		P_DECLARE_PRIVATE(QAMQP::Connection)
		Q_DISABLE_COPY(Connection)
		Connection(Client * parent = 0);
	public:		
		~Connection();


		void addCustomProperty(const QString & name, const QString & value);
		QString customProperty(const QString & name) const;

		void startOk();
		void secureOk();
		void tuneOk();
		void open();
		void close(int code, const QString & text, int classId = 0, int methodId = 0);
		void closeOk();	

		bool isConnected() const;

		void setQOS(qint32 prefetchSize, quint16 prefetchCount);

	Q_SIGNALS:
		void disconnected();
		void connected();
	protected:
		ConnectionPrivate * const pd_ptr;

	private:
		void openOk();
		friend class ClientPrivate;
		friend class ChannelPrivate;

		void _q_method(const QAMQP::Frame::Method & frame);
		Q_PRIVATE_SLOT(pd_func(), void _q_heartbeat());
	};
}

// Include private header so MOC won't complain
#ifdef QAMQP_P_INCLUDE
# include "amqp_connection_p.h"
#endif

#endif // amqp_connection_h__
