#ifndef amqp_channel_p_h__
#define amqp_channel_p_h__

#include <QtCore/private/qobject_p.h>

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{
	class Client;
	class ClientPrivate;
	class ChannelPrivate : public QObjectPrivate
	{
		Q_DECLARE_PUBLIC(QAMQP::Channel)
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
			bmgetEmpty = 72,
			bmAck = 80,
			bmReject = 90,
			bmRecoverAsync = 100,
			METHOD_ID_ENUM(bmRecover, 110)
		};

		ChannelPrivate(int version = QObjectPrivateVersion);
		~ChannelPrivate();

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

		virtual void _q_method(const QAMQP::Frame::Method & frame);
		void _q_open();


		void sendFrame(const QAMQP::Frame::Base & frame);

		QPointer<Client> client_;

		QString name;
		int number;

		static int nextChannelNumber_;
		bool opened;
		bool needOpen;
	};
}
#endif // amqp_channel_p_h__