#ifndef amqp_channel_h__
#define amqp_channel_h__

#include <QObject>
#include "amqp_global.h"
#include "amqp_frame.h"

namespace QAMQP
{
	class ChannelPrivate;
	class Client;
	class Channel : public QObject, public Frame::MethodHandler
	{
		Q_OBJECT

		Q_PROPERTY(int number READ channelNumber);
		Q_PROPERTY(QString name READ name WRITE setName);		

		P_DECLARE_PRIVATE(QAMQP::Channel)
		Q_DISABLE_COPY(Channel)		
	public:		
		~Channel();

		void closeChannel();
		void reopen();

		QString name();
		int channelNumber();		
				
		void setName(const QString &name);
		void setQOS(qint32 prefetchSize, quint16 prefetchCount);
		bool isOpened() const;

	signals:
		void opened();
		void closed();
		void flowChanged(bool enabled);

	protected:
		Channel(int channelNumber = -1, Client * parent = 0);
		Channel(ChannelPrivate * d);
		virtual void onOpen();
		virtual void onClose();

		ChannelPrivate * const pd_ptr;

	private:
		void stateChanged(int state);
		friend class ClientPrivate;
		void _q_method(const QAMQP::Frame::Method & frame);

		Q_PRIVATE_SLOT(pd_func(), void _q_open())
		Q_PRIVATE_SLOT(pd_func(), void _q_disconnected())
	};
}

#ifdef QAMQP_P_INCLUDE
# include "amqp_channel_p.h"
#endif
#endif // amqp_channel_h__
