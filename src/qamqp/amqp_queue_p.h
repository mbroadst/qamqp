#ifndef amqp_queue_p_h__
#define amqp_queue_p_h__

#include "amqp_channel_p.h"
#define METHOD_ID_ENUM(name, id) name = id, name ## Ok
#include <QQueue>

namespace QAMQP
{
	using namespace QAMQP::Frame;
	class QueuePrivate: public ChannelPrivate
	{
		P_DECLARE_PUBLIC(QAMQP::Queue)
	public:

		enum MethodId
		{
			METHOD_ID_ENUM(miDeclare, 10),
			METHOD_ID_ENUM(miBind, 20),
			METHOD_ID_ENUM(miUnbind, 50),
			METHOD_ID_ENUM(miPurge, 30),
			METHOD_ID_ENUM(miDelete, 40)
		};

		QueuePrivate(Queue * q);
		~QueuePrivate();

		void declare();
		void remove(bool ifUnused = true, bool ifEmpty = true, bool noWait = true);
		void purge();
		void bind(const QString & exchangeName, const QString & key);
		void unbind(const QString & exchangeName, const QString & key);

		void declareOk(const QAMQP::Frame::Method & frame);
		void deleteOk(const QAMQP::Frame::Method & frame);
		void bindOk(const QAMQP::Frame::Method & frame);
		void unbindOk(const QAMQP::Frame::Method & frame);

		/************************************************************************/
		/* CLASS BASIC METHODS                                                  */
		/************************************************************************/

		void consume(Queue::ConsumeOptions options);
		void consumeOk(const QAMQP::Frame::Method & frame);		
		void deliver(const QAMQP::Frame::Method & frame);

		void get();		
		void getOk(const QAMQP::Frame::Method & frame);
		void ack(const MessagePtr & Message);
		
		QString type;
		Queue::QueueOptions options;

		bool _q_method(const QAMQP::Frame::Method & frame);

		bool delayedDeclare;
		bool declared;
		bool noAck;
		QString consumerTag;

		QQueue<QPair<QString, QString> > delayedBindings;
		QQueue<QAMQP::MessagePtr> messages_;

		bool recievingMessage;

		void _q_content(const QAMQP::Frame::Content & frame);
		void _q_body(const QAMQP::Frame::ContentBody & frame);
	};	
}
#endif // amqp_queue_p_h__
