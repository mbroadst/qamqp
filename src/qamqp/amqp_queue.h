#ifndef amqp_queue_h__
#define amqp_queue_h__

#include "amqp_channel.h"

namespace QAMQP
{
	class Client;
	class ClientPrivate;
	class Exchange;
	class QueuePrivate;
	class Queue : public Channel
	{
		Q_OBJECT
		Queue(int channelNumber = -1, Client * parent = 0);

		Q_PROPERTY(QueueOptions option READ option );
		Q_PROPERTY(QString consumerTag READ consumerTag WRITE setConsumerTag)
		
		Q_DECLARE_PRIVATE(QAMQP::Queue)
		Q_DISABLE_COPY(Queue);	
		friend class ClientPrivate;

	protected:
		void onOpen();
		void onClose();
	
	public:
		enum QueueOption {
			NoOptions = 0x0,
			Passive = 0x01,
			Durable = 0x02,
			Exclusive = 0x4,
			AutoDelete = 0x8,			
			NoWait = 0x10
		};
		Q_DECLARE_FLAGS(QueueOptions, QueueOption)

		enum ConsumeOption {
				coNoLocal = 0x1,
				coNoAck = 0x02,
				coExclusive = 0x04,	
				coNoWait = 0x8
		};
		Q_DECLARE_FLAGS(ConsumeOptions, ConsumeOption)

		~Queue();

		QueueOptions option() const;

		void declare();
		void declare(const QString &name, QueueOptions options);
		void remove(bool ifUnused = true, bool ifEmpty = true, bool noWait = true);

		void purge();

		void bind(const QString & exchangeName, const QString & key);
		void bind(Exchange * exchange, const QString & key);

		void unbind(const QString & exchangeName, const QString & key);
		void unbind(Exchange * exchange, const QString & key);

		void get();
		void consume(ConsumeOptions options = NoOptions);
		void setConsumerTag(const QString &consumerTag);
		QString consumerTag() const;
	
	Q_SIGNALS:
		void declared();
		void binded(bool);
		void removed();
	private:
		Q_PRIVATE_SLOT(d_func(), void _q_content(const QAMQP::Frame::Content & frame))
		Q_PRIVATE_SLOT(d_func(), void _q_body(int channeNumber, const QByteArray & body))
	};
}
#ifdef QAMQP_P_INCLUDE
# include "amqp_queue_p.h"
#endif
#endif // amqp_queue_h__