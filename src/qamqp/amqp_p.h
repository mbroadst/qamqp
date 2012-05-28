#ifndef qamqp_amqp_p_h__
#define qamqp_amqp_p_h__

#include <QSharedPointer>

#include "amqp_global.h"
#include "amqp_network.h"
#include "amqp_connection.h"
#include "amqp_authenticator.h"

namespace QAMQP
{
	class ClientPrivate
	{
		P_DECLARE_PUBLIC(QAMQP::Client)

	public:
		ClientPrivate(Client * q ) ;
		~ClientPrivate();


		void init(QObject * parent);
		void init(QObject * parent, const QUrl & connectionString);
		void printConnect() const;
		void connect();
		void disconnect();
		void parseCnnString( const QUrl & connectionString);
		void sockConnect();
		void login();
		void setAuth(Authenticator* auth);
		Exchange * createExchange(int channelNumber, const QString &name);
		Queue * createQueue(int channelNumber, const QString &name);

		quint32 port;
		QString host;
		QString virtualHost;

		QPointer<QAMQP::Network> network_;	
		QPointer<QAMQP::Connection> connection_;
		QSharedPointer<Authenticator> auth_;

		bool isSSl() const;

		

		Client * const pq_ptr;

	};

}
#endif // amqp_p_h__
