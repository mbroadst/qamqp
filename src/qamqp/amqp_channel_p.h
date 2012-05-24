#ifndef amqp_channel_p_h__
#define amqp_channel_p_h__

#include "amqp_global.h"
#include <QPointer>

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{
	class Client;
	class ClientPrivate;
	class ChannelPrivate 
	{
		P_DECLARE_PUBLIC(QAMQP::Channel)
	public:
		enum MethodId
		{
			METHOD_ID_ENUM(miOpen, 10),
			METHOD_ID_ENUM(miFlow, 20),
			METHOD_ID_ENUM(miClose, 40)
		};

		enum State {
			csOpened,
			csClosed,
			csIdle,
			csRunning
		};

		enum BasicMethod
		{
			METHOD_ID_ENUM(bmQos, 10),
			METHOD_ID_ENUM(bmConsume, 20),
			METHOD_ID_ENUM(bmCancel, 30),
			bmPublish = 40,
			bmReturn = 50,
			bmDeliver = 60,
			METHOD_ID_ENUM(bmGet, 70),
			bmGetEmpty = 72,
			bmAck = 80,
			bmReject = 90,
			bmRecoverAsync = 100,
			METHOD_ID_ENUM(bmRecover, 110)
		};

		ChannelPrivate(Channel * q);
		virtual ~ChannelPrivate();

		void init(int channelNumber, Client * parent);

		void open();
		void flow();
		void flowOk();
		void close(int code, const QString & text, int classId, int methodId);
		void closeOk();

		//////////////////////////////////////////////////////////////////////////

		void openOk(const QAMQP::Frame::Method & frame);
		void flow(const QAMQP::Frame::Method & frame);
		void flowOk(const QAMQP::Frame::Method & frame);
		void close(const QAMQP::Frame::Method & frame);
		void closeOk(const QAMQP::Frame::Method & frame);

		virtual bool _q_method(const QAMQP::Frame::Method & frame);
		virtual void _q_disconnected();
		void _q_open();
		
		void setQOS(qint32 prefetchSize, quint16 prefetchCount);
		void sendFrame(const QAMQP::Frame::Base & frame);

		QPointer<Client> client_;

		QString name;
		int number;

		static int nextChannelNumber_;
		bool opened;
		bool needOpen;

		Channel * const pq_ptr;
	};
}
#endif // amqp_channel_p_h__