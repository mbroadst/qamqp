#ifndef amqp_exchange_h__
#define amqp_exchange_h__

#include "amqp_channel.h"
namespace QAMQP
{
	class Client;
	class ClientPrivate;
	class Exchange : public Channel
	{
		Q_OBJECT;
		Exchange(Client * parent = 0) : Channel(parent) {}
	public:		
		friend class ClientPrivate;
		~Exchange(){}
	};
}
#endif // amqp_exchange_h__