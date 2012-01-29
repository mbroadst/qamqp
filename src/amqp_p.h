#ifndef qamqp_amqp_p_h__
#define qamqp_amqp_p_h__
#include <QtCore/private/qobject_p.h>


#include "amqp_network.h"
#include "amqp_connection.h"

namespace QAMQP
{
	class ClientPrivate : public QObjectPrivate
	{
		Q_DECLARE_PUBLIC(QAMQP::Client)
	public:
		ClientPrivate(int version = QObjectPrivateVersion);
		~ClientPrivate();

		void init(QObject * parent);
		void init(QObject * parent, const QUrl & connectionString);
		void printConnect() const;
		void connect();
		void parseCnnString( const QUrl & connectionString);
		void sockConnect();
		void login();

		Exchange * createExchange(const QString &name);
		Queue * createQueue(const QString &name);

		quint32 port;
		QString host;
		QString virtualHost;
		QString user;
		QString password;
		QPointer<QAMQP::Network> network_;	
		QPointer<QAMQP::Connection> connection_;
	};
}
#endif // amqp_p_h__
