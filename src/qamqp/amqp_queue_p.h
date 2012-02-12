#include "amqp_channel_p.h"
#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{
	using namespace QAMQP::Frame;
	class QueuePrivate: public ChannelPrivate
	{
		Q_DECLARE_PUBLIC(QAMQP::Queue)
	public:

		enum MethodId
		{
			METHOD_ID_ENUM(miDeclare, 10),
			METHOD_ID_ENUM(miBind, 20),
			METHOD_ID_ENUM(miUnbind, 50),
			METHOD_ID_ENUM(miPurge, 30),
			METHOD_ID_ENUM(miDelete, 40)
		};

		QueuePrivate();
		~QueuePrivate();

		void declare();
		void remove(bool ifUnused = true, bool ifEmpty = true, bool noWait = true);
		void purge();
		void bind(const QString & exchangeName, const QString & key);
		void unbind(const QString & exchangeName, const QString & key);

		void setConsumerTag( const QString &consumerTag );

		void declareOk(const QAMQP::Frame::Method & frame);
		void deleteOk(const QAMQP::Frame::Method & frame);
		void bindOk(const QAMQP::Frame::Method & frame);
		void unbindOk(const QAMQP::Frame::Method & frame);

		
		QString type;
		Queue::QueueOptions options;

		void _q_method(const QAMQP::Frame::Method & frame);

		bool deleyedDeclare;
		bool declared;
		QString consumerTag;

		QMap<QString, QString> delayedBindings;
	};	


}