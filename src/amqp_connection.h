#ifndef amqp_connection_h__
#define amqp_connection_h__

#include <QObject>
#include "amqp_frame.h"
#include "qamqp_global.h"

namespace QAMQP
{
	class ConnectionPrivate;
	class ClientPrivate;
	class Client;
	class Connection : public QObject
	{
		Q_OBJECT
		Q_DECLARE_PRIVATE(QAMQP::Connection)
		Q_DISABLE_COPY(Connection)
		Connection(Client * parent = 0);
	public:		
		~Connection();

		void startOk();
		void secureOk();
		void tuneOk();
		void open();
		void close(int code, const QString & text, int classId = 0, int methodId = 0);
		void closeOk();	
	Q_SIGNALS:
		void disconnected();
		void connected();
	protected:
		Connection(ConnectionPrivate &dd, Client* parent);
	private:
		void openOk();
		friend class ClientPrivate;
		Q_PRIVATE_SLOT(d_func(), void _q_method(const QAMQP::Frame::Method & frame))
	};
}

// Include private header so MOC won't complain
#ifdef QAMQP_P_INCLUDE
# include "amqp_connection_p.h"
#endif

#endif // amqp_connection_h__