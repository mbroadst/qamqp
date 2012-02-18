#ifndef amqp_frame_h__
#define amqp_frame_h__

#include <QDataStream>
#include <QHash>
#include <QVariant>

#define AMQP_BASIC_CONTENT_TYPE_FLAG (1 << 15)
#define AMQP_BASIC_CONTENT_ENCODING_FLAG (1 << 7)
#define AMQP_BASIC_HEADERS_FLAG (1 << 13)
#define AMQP_BASIC_DELIVERY_MODE_FLAG (1 << 12)
#define AMQP_BASIC_PRIORITY_FLAG (1 << 11)
#define AMQP_BASIC_CORRELATION_ID_FLAG (1 << 10)
#define AMQP_BASIC_REPLY_TO_FLAG (1 << 9)
#define AMQP_BASIC_EXPIRATION_FLAG (1 << 8)
#define AMQP_BASIC_MESSAGE_ID_FLAG (1 << 14)
#define AMQP_BASIC_TIMESTAMP_FLAG (1 << 6)
#define AMQP_BASIC_TYPE_FLAG (1 << 5)
#define AMQP_BASIC_USER_ID_FLAG (1 << 4)
#define AMQP_BASIC_APP_ID_FLAG (1 << 3)
#define AMQP_BASIC_CLUSTER_ID_FLAG (1 << 2)

namespace QAMQP
{
	namespace Frame
	{
		enum Type
		{
			ftMethod = 1,
			ftHeader = 2,
			ftBody = 3,
			ftHeartbeat = 8
		};

		enum MethodClass
		{
			fcConnection = 10,
			fcChannel = 20,
			fcExchange = 40,
			fcQueue = 50,
			fcBasic = 60,
			fcTx = 90,
		};

		struct decimal
		{
			qint8 scale;
			quint32 value;

		};
		Q_DECLARE_METATYPE(QAMQP::Frame::decimal);

		typedef QHash<QString, QVariant> TableField;
		Q_DECLARE_METATYPE(QAMQP::Frame::TableField);

		QDataStream & serialize( QDataStream & stream, const QAMQP::Frame::TableField & f );
		QDataStream & deserialize( QDataStream & stream, QAMQP::Frame::TableField & f );
		QVariant readField( qint8 valueType, QDataStream &s );
		void writeField( QDataStream &s, const QVariant & value );
		void writeField( qint8 valueType, QDataStream &s, const QVariant & value, bool withType = false );
		void print( const QAMQP::Frame::TableField & f );

		class Base
		{
		public:
			Base(Type type);
			Base(QDataStream& raw);
			Type type() const;
			void setChannel(qint16 channel);
			qint16 channel() const;
			virtual qint32 size() const;
			void toStream(QDataStream & stream) const;
		protected:
			void writeHeader(QDataStream & stream) const;
			virtual void writePayload(QDataStream & stream) const;
			void writeEnd(QDataStream & stream) const;

			void readHeader(QDataStream & stream);
			virtual void readPayload(QDataStream & stream);
			void readEnd(QDataStream & stream);

			qint32 size_;
		private:
			qint8 type_;
			qint16 channel_;
			
		};

		class Method : public Base
		{
		public:
			Method();
			Method(MethodClass methodClass, qint16 id);
			Method(QDataStream& raw);

			MethodClass methodClass() const;
			qint16 id() const;
			qint32 size() const;
			void setArguments(const QByteArray & data);
			QByteArray arguments() const;

		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);
			short methodClass_;
			qint16 id_;
			QByteArray arguments_;
		};

		class Content : public Base
		{
		public:

			enum Property
			{
				cpContentType = AMQP_BASIC_CONTENT_TYPE_FLAG,
				cpContentEncoding = AMQP_BASIC_CONTENT_ENCODING_FLAG,
				cpHeaders = AMQP_BASIC_HEADERS_FLAG,
				cpDeliveryMode = AMQP_BASIC_DELIVERY_MODE_FLAG,
				cpPriority = AMQP_BASIC_PRIORITY_FLAG,
				cpCorrelationId = AMQP_BASIC_CORRELATION_ID_FLAG,
				cpReplyTo = AMQP_BASIC_REPLY_TO_FLAG,
				cpExpiration = AMQP_BASIC_EXPIRATION_FLAG,
				cpMessageId = AMQP_BASIC_MESSAGE_ID_FLAG,
				cpTimestamp = AMQP_BASIC_TIMESTAMP_FLAG,
				cpType = AMQP_BASIC_TYPE_FLAG,
				cpUserId = AMQP_BASIC_USER_ID_FLAG,
				cpAppId = AMQP_BASIC_APP_ID_FLAG,
				cpClusterID = AMQP_BASIC_CLUSTER_ID_FLAG
			};
			Q_DECLARE_FLAGS(Properties, Property)

			Content();
			Content(MethodClass methodClass);
			Content(QDataStream& raw);

			MethodClass methodClass() const;
			qint32 size() const;
			void setProperty(Property prop, const QVariant & value);
			QVariant property(Property prop) const;

			void setBody(const QByteArray & data);
			QByteArray body() const;
			qlonglong bodySize() const;

		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);
			short methodClass_;
			qint16 id_;
			QByteArray body_;
			mutable QByteArray buffer_;
			QHash<int, QVariant> properties_;
			qlonglong bodySize_;
		};

		class ContentBody : public Base
		{
		public:
			ContentBody();
			void setBody(const QByteArray & data);
			QByteArray body() const;
			qint32 size() const;
		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);

		private:
			QByteArray body_;
		};
	}
}

#endif // amqp_frame_h__