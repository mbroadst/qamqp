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

/*!
Library namespace
@namespace QAMQP
*/
namespace QAMQP
{
	class QueuePrivate;
	/*!
	Frame namespace
	@namespace Frame
	*/
	namespace Frame
	{
		typedef quint16 channel_t;
		/*!
		@brief Header size in bytes
		*/
		static const qint64 HEADER_SIZE = 7;
		/*!
		@brief Frame end indicator size in bytes
		*/
		static const qint64 FRAME_END_SIZE = 1;

		/*!
		@brief Frame end marker
		*/
		static const quint8 FRAME_END = 0xCE;

		/*!
		@brief Frame type
		*/
		enum Type
		{
			ftMethod = 1, /*!< Used define method frame */
			ftHeader = 2, /*!< Used define content header frame */
			ftBody = 3,   /*!< Used define content body frame */
			ftHeartbeat = 8 /*!< Used define heartbeat frame */
		};
	
		/*!
		@brief Frame method class
		@enum MethodClass
		*/
		enum MethodClass
		{
			fcConnection = 10, /*!< Define class of methods related to connection  */
			fcChannel = 20,  /*!< Define class of methods related to channel  */
			fcExchange = 40, /*!< Define class of methods related to exchange  */
			fcQueue = 50, /*!< Define class of methods related to queue  */
			fcBasic = 60, /*!< Define class of methods related to basic command  */
			fcTx = 90,
		};

		struct decimal
		{
			qint8 scale;
			quint32 value;

		};

		/*!
		@brief Definition implementation of TableField type
		@detailed Define implementation TableField type in builtin Qt types. Key contains field name, value contains field data.
		It can by any type witch support serialization in AMQP types.
		*/
		typedef QHash<QString, QVariant> TableField;

		QDataStream & serialize( QDataStream & stream, const QAMQP::Frame::TableField & f );
		QDataStream & deserialize( QDataStream & stream, QAMQP::Frame::TableField & f );
		QVariant readField( qint8 valueType, QDataStream &s );
		void writeField( QDataStream &s, const QVariant & value );
		void writeField( qint8 valueType, QDataStream &s, const QVariant & value, bool withType = false );
		void print( const QAMQP::Frame::TableField & f );

		/*!
		@brief Base class for any frames.
		@detailed Implement main methods for serialize and deserialize raw frame data.
		All frames start with a 7-octet header composed of a type field (octet), a channel field (short integer) and a
		size field (long integer):
		@code Frame struct
		  0	  1		 3	   7				  size+7	size+8
		  +------+---------+---------+ +-------------+ +-----------+
		  | type | channel | size	| | payload	 | | frame-end |
		  +------+---------+---------+ +-------------+ +-----------+
		@endcode
		octet short long 'size' octets octet
		*/			
		class Base
		{
		public:
			/*!
			Base class constructor.
			@detailed Construct frame class for sending.
			@param type Define type of constructed frame.
			*/
			Base(Type type);

			/*!
			Base class constructor.
			@detailed Construct frame class from received raw data.
			@param raw Data stream for reading source data.
			*/
			Base(QDataStream& raw);

			/*!
			Base class virtual destructor
			*/
			virtual ~Base();

			/*!
			Frame type
			@detailed Return type of current frame.
			*/
			Type type() const;
			
			/*!
			Set number of associated channel.
			@param channel Number of channel.
			@sa channel()
			*/
			void setChannel(qint16 channel);
			
			/*!
			Return number of associated channel.
			@sa setChannel()
			*/
			qint16 channel() const;

			/*!
			Return size of frame.
			*/
			virtual qint32 size() const;

			/*!
			Output frame to stream.
			@param stream Stream for serilize frame.
			*/
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

		/*!
		@brief Class for working with method frames.
		@detailed Implement main methods for serialize and deserialize raw method frame data.
		Method frame bodies consist of an invariant list of data fields, called "arguments". All method bodies start
		with identifier numbers for the class and method:
		@code Frame struct
		0		   2		   4
		+----------+-----------+-------------- - -
		| class-id | method-id | arguments...
		+----------+-----------+-------------- - -
			short	  short	...
		@endcode
		*/
		class Method : public Base
		{
		public:
			/*!
			Method class constructor.
			@detailed Construct frame class for sending.
			*/
			Method();

			/*!
			Method class constructor.
			@detailed Construct frame class for sending.
			@param methodClass Define method class id of constructed frame.
			@param id Define method id of constructed frame.
			*/
			Method(MethodClass methodClass, qint16 id);

			/*!
			Method class constructor.
			@detailed Construct frame class from received raw data.
			@param raw Data stream for reading source data.
			*/
			Method(QDataStream& raw);

			/*!
			Method class type.
			*/
			MethodClass methodClass() const;

			/*!
			Method id.
			*/
			qint16 id() const;
			qint32 size() const;

			/*!
			Set arguments for method.
			@param data Serialized method arguments.
			@sa arguments
			*/
			void setArguments(const QByteArray & data);

			/*!
			Return arguments for method.
			@sa setArguments
			*/
			QByteArray arguments() const;

		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);
			short methodClass_;
			qint16 id_;
			QByteArray arguments_;
		};


		/*!
		@brief Class for working with content frames.
		@detailed Implement main methods for serialize and deserialize raw content frame data.
		A content header payload has this format:
		@code Frame struct
		+----------+--------+-----------+----------------+------------- - -
		| class-id | weight | body size | property flags | property list...
		+----------+--------+-----------+----------------+------------- - -
		   short	 short   long long		short		remainder...
		@endcode
		
		| Property | Description |
		| ---------- | ----------- |
		|cpContentType | MIME content type |
		| ocpContentEncoding | MIME content encoding |
		| cpHeaders | message header field table |
		| cpDeliveryMode| nonpersistent (1) or persistent (2) |
		| cpPriority | message priority, 0 to 9 |
		| cpCorrelationId | application correlation identifier |
		| cpReplyTo | address to reply to |
		| cpExpiration | message expiration specification |
		| cpMessageId | application message identifier |
		| cpTimestamp | message timestamp |
		| cpType | message type name |
		| cpUserId | creating user id |
		| cpAppId | creating application id |
		| cpClusterID| cluster ID | 	

		Default property:
		@sa setProperty
		@sa property

		*/
		class Content : public Base
		{
			friend class QAMQP::QueuePrivate;
		public:

			/*! Default content frame property
			
			*/
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
			Q_DECLARE_FLAGS(Properties, Property);

			/*!
			Content class constructor.
			@detailed Construct frame content header class for sending.
			*/
			Content();

			/*!
			Content class constructor.
			@detailed Construct frame content header class for sending.
			@param methodClass Define method class id of constructed frame.
			*/
			Content(MethodClass methodClass);

			/*!
			Content class constructor.
			@detailed Construct frame content header class for sending.
			@param raw Data stream for reading source data.
			*/
			Content(QDataStream& raw);

			/*!
			Method class type.
			*/
			MethodClass methodClass() const;
			qint32 size() const;

			/*!
			Set default content header property
			@param prop Any default content header property
			@param value Associated data
			*/
			void setProperty(Property prop, const QVariant & value);

			/*!
			Return associated with property value
			@param prop Any default content header property
			*/
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
			ContentBody(QDataStream& raw);
			void setBody(const QByteArray & data);
			QByteArray body() const;
			qint32 size() const;
		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);

		private:
			QByteArray body_;
		};

		/*!
		@brief Class for working with heartbeat frames.
		@detailed Implement frame for heartbeat send.		
		*/
		class Heartbeat : public Base
		{
		public:
			/*!
			Heartbeat class constructor.
			@detailed Construct frame class for sending.
			*/
			Heartbeat();			

		protected:
			void writePayload(QDataStream & stream) const;
			void readPayload(QDataStream & stream);
		};

		class MethodHandler
		{
		public:
			virtual void _q_method(const QAMQP::Frame::Method & frame) = 0;
		};

		class ContentHandler
		{
		public:
			virtual void _q_content(const QAMQP::Frame::Content & frame) = 0;
		};

		class ContentBodyHandler
		{
		public:
			virtual void _q_body(const QAMQP::Frame::ContentBody & frame) = 0;
		};
	}
}

Q_DECLARE_METATYPE(QAMQP::Frame::decimal);
Q_DECLARE_METATYPE(QAMQP::Frame::TableField);

#endif // amqp_frame_h__
