#ifndef amqp_connection_p_h__
#define amqp_connection_p_h__

#include <QtCore/private/qobject_p.h>

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{
	class Client;
	class ClientPrivate;
	class ConnectionPrivate : public QObjectPrivate
	{
		Q_DECLARE_PUBLIC(QAMQP::Connection)
	public:
		enum MethodId
		{
			METHOD_ID_ENUM(miStart, 10),
			METHOD_ID_ENUM(miSecure, 20),
			METHOD_ID_ENUM(miTune, 30),
			METHOD_ID_ENUM(miOpen, 40),
			METHOD_ID_ENUM(miClose, 50)
		};

		ConnectionPrivate(int version = QObjectPrivateVersion);
		~ConnectionPrivate();
		void init(Client * parent);
		void startOk();
		void secureOk();
		void tuneOk();
		void open();
		void close(int code, const QString & text, int classId = 0, int methodId = 0);
		void closeOk();

		void start(const QAMQP::Frame::Method & frame);
		void secure(const QAMQP::Frame::Method & frame);
		void tune(const QAMQP::Frame::Method & frame);
		void openOk(const QAMQP::Frame::Method & frame);
		void close(const QAMQP::Frame::Method & frame);
		void closeOk(const QAMQP::Frame::Method & frame);
		void _q_method(const QAMQP::Frame::Method & frame);

		void setQOS(qint32 prefetchSize, quint16 prefetchCount, int channel, bool global);

		QPointer<Client> client_;
		bool closed_;
		bool connected;
	};
}
#endif // amqp_connection_p_h__