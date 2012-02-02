#ifndef amqp_frame_h__
#define amqp_frame_h__

#include <QDataStream>
#include <QHash>
#include <QVariant>
#include <QSharedPointer>

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

		enum FieldValueKind {
			fkBoolean = 't',
			fkI8 = 'b',
			fkU8 = 'B',
			fkI16 = 'U',
			fkU16 = 'u',
			fkI32 = 'I',
			fkU32 = 'i',
			fkI64 = 'l',
			fkU64 = 'L',
			fkFloat = 'f',
			fkDouble = 'd',
			fkDecimal = 'D',
			fkLongString = 'S',
			fkShortString = 's',
			fkArray = 'A',
			fkTimestamp = 'T',
			fkTable = 'F',
			fkVoid = 'V',
			fkBytes = 'x'
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
		QVariant readField( FieldValueKind valueType, QDataStream &s );
		void writeField( QDataStream &s, const QVariant & value );
		void writeField( FieldValueKind valueType, QDataStream &s, const QVariant & value, bool withType = false );
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

		typedef QSharedPointer<QAMQP::Frame::Base> BasePtr;

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
	}
}

#endif // amqp_frame_h__