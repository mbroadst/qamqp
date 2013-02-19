#include "amqp_channel_p.h"
#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{
	using namespace QAMQP::Frame;
	class ExchangePrivate: public ChannelPrivate
	{
		P_DECLARE_PUBLIC(QAMQP::Exchange)
	public:

		enum MethodId
		{
			METHOD_ID_ENUM(miDeclare, 10),
			METHOD_ID_ENUM(miDelete, 20)
		};

		ExchangePrivate(Exchange * q);
		~ExchangePrivate();

		void declare();
		void remove(bool ifUnused = true, bool noWait = true);

		void declareOk(const QAMQP::Frame::Method & frame);
		void deleteOk(const QAMQP::Frame::Method & frame);

		void publish(const QByteArray & message, const QString & key, const QString &mimeType = QString::fromLatin1("text/plain"), const QVariantHash & headers = QVariantHash(), const Exchange::MessageProperties & properties = Exchange::MessageProperties());

		QString type;
		Exchange::ExchangeOptions options;
		TableField arguments;
		
		bool _q_method(const QAMQP::Frame::Method & frame);
		void _q_disconnected();
		
		bool delayedDeclare;
		bool declared;

	};	
}
