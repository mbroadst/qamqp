#ifndef amqp_channel_h__
#define amqp_channel_h__

#include <QObject>
#include "qamqp_global.h"
#include "amqp_frame.h"

namespace QAMQP
{
	class ChannelPrivate;
	class Client;
	class Channel : public QObject
	{
		Q_OBJECT

		Q_PROPERTY(int number READ channelNumber);
		Q_PROPERTY(QString name READ name WRITE setName);

		Q_DECLARE_PRIVATE(QAMQP::Channel)
		Q_DISABLE_COPY(Channel)		
	public:		
		~Channel();

		void closeChannel();
		void reopen();

		QString name();
		int channelNumber();		
				
		void setParam(int param);
		void setName(const QString &name);

	signals:
		void opened();
		void closed();
		void flowChanged(bool enabled);

	protected:
		Channel(Client * parent = 0);
		Channel(ChannelPrivate &dd, Client* parent);

	private:
		void stateChanged(int state);
		friend class ClientPrivate;
		Q_PRIVATE_SLOT(d_func(), void _q_open())
		Q_PRIVATE_SLOT(d_func(), void _q_method(const QAMQP::Frame::Method & frame))
	};
}

#ifdef QAMQP_P_INCLUDE
# include "amqp_channel_p.h"
#endif
#endif // amqp_channel_h__