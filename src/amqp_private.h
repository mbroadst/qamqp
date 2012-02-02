#ifndef amqp_private_h__
#define amqp_private_h__

#include <QtCore/private/qobject_p.h>
#include <QDataStream>
#include <QBuffer>

#include "amqp_frame.h"
namespace QAMQP
{
	class Base: public QObjectPrivate
	{
	public:
		Base(int version = QObjectPrivateVersion);
		~Base();
		void init(QAMQP::Client * client);
		virtual QAMQP::Frame::MethodClass methodClass() const = 0;
		void startMethod(int id);
		void writeArgument(QAMQP::Frame::FieldValueKind type, QVariant value);
		void writeArgument(QVariant value);
		void endWrite();

		void startRead(QAMQP::Frame::BasePtr frame);
		QVariant readArgument(QAMQP::Frame::FieldValueKind type);
		void endRead();

	private:
		QAMQP::Frame::BasePtr inFrame_;
		QAMQP::Frame::BasePtr outFrame_;
		QDataStream streamIn_;
		QDataStream streamOut_;
		QSharedPointer<QBuffer> outArguments_;
		QPointer<Client> client_;

	};

}
#endif // amqp_private_h__