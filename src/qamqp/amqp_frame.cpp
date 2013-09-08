#include "amqp_frame.h"

#include <QDateTime>
#include <QList>
#include <QDebug>
#include <float.h>

using namespace QAMQP::Frame;
Base::Base( Type type ) : size_(0), type_(type), channel_(0) {}

Base::Base( QDataStream& raw )
{
	readHeader(raw);
}

Type Base::type() const
{
	return Type(type_);
}

Base::~Base()
{}

void Base::setChannel( qint16 channel )
{
	channel_ = channel;
}

qint16 Base::channel() const
{
	return channel_;
}

qint32 Base::size() const
{
	return 0;
}

void QAMQP::Frame::Base::writeHeader( QDataStream & stream ) const
{
	stream << type_;
	stream << channel_;
	stream << qint32(size());

}

void QAMQP::Frame::Base::writeEnd( QDataStream & stream ) const
{
	stream << qint8(FRAME_END);
}

void QAMQP::Frame::Base::writePayload( QDataStream & ) const{}

void QAMQP::Frame::Base::readHeader( QDataStream & stream )
{
	stream >> type_;
	stream >> channel_;
	stream >> size_;
	
}

void QAMQP::Frame::Base::readEnd( QDataStream & stream )
{
	unsigned char end_  = 0;
	stream.readRawData(reinterpret_cast<char*>(&end_), sizeof(end_));
	if(end_ != FRAME_END )
	{
		qWarning("Wrong end of frame");
	}
}

void QAMQP::Frame::Base::readPayload( QDataStream & stream )
{
	stream.skipRawData(size_);
}

void QAMQP::Frame::Base::toStream( QDataStream & stream ) const
{
	writeHeader(stream);
	writePayload(stream);
	writeEnd(stream);
}

//////////////////////////////////////////////////////////////////////////


QAMQP::Frame::Method::Method( MethodClass methodClass, qint16 id )
: Base(ftMethod), methodClass_(methodClass), id_(id)
{

}

QAMQP::Frame::Method::Method( QDataStream& raw )
: Base(raw)
{
	readPayload(raw);
}

QAMQP::Frame::Method::Method(): Base(ftMethod)
{

}

MethodClass QAMQP::Frame::Method::methodClass() const
{
	return MethodClass(methodClass_);
}

qint16 QAMQP::Frame::Method::id() const
{
	return id_;
}

qint32 QAMQP::Frame::Method::size() const
{
	return sizeof(id_) + sizeof(methodClass_) + arguments_.size();
}

void QAMQP::Frame::Method::setArguments( const QByteArray & data )
{
	arguments_ = data;
}

QByteArray QAMQP::Frame::Method::arguments() const
{
	return arguments_;
}

void QAMQP::Frame::Method::readPayload( QDataStream & stream )
{
	stream >> methodClass_;
	stream >> id_;
	
	arguments_.resize(size_ - (sizeof(id_) + sizeof(methodClass_)));
	stream.readRawData(arguments_.data(), arguments_.size());
}

void QAMQP::Frame::Method::writePayload( QDataStream & stream ) const
{
	stream << quint16(methodClass_);
	stream << quint16(id_);
	stream.writeRawData(arguments_.data(), arguments_.size());
}


//////////////////////////////////////////////////////////////////////////


QVariant QAMQP::Frame::readField( qint8 valueType, QDataStream &s )
{
	QVariant value;
	QByteArray tmp;
	qint8 nameSize_ = 0;
	char octet = 0;	

	switch(valueType)
	{
	case 't':
		s.readRawData(&octet, sizeof(octet));
		value = QVariant::fromValue<bool>(octet > 0);
		break;
	case 'b':
		s.readRawData(&octet, sizeof(octet));
		value = QVariant::fromValue<int>(octet);
		break;
	case 'B':
		s.readRawData(&octet, sizeof(octet));
		value = QVariant::fromValue<uint>(octet);
		break;
	case 'U':
		{
			qint16 tmp_value_ = 0;
			s >> tmp_value_;
			value = QVariant::fromValue<int>(tmp_value_);
			break;
		}
	case 'u':
		{
			quint16 tmp_value_ = 0;
			s >> tmp_value_;
			value = QVariant::fromValue<uint>(tmp_value_);
			break;
		}
	case 'I':
		{
			qint32 tmp_value_ = 0;
			s >> tmp_value_;
			value = QVariant::fromValue<int>(tmp_value_);
			break;
		}
	case 'i':
		{
			quint32 tmp_value_ = 0;
			s >> tmp_value_;
			value = QVariant::fromValue<uint>(tmp_value_);
			break;
		}
	case 'L':
		{
			qlonglong v = 0 ;
			s >> v;
			value = v;
		}
		
		break;
	case 'l':
		{
			qulonglong v = 0 ;
			s >> v;
			value = v;
		}
		
		break;
	case 'f':
		{
			float tmp_value_;
			s >> tmp_value_;
			value = QVariant::fromValue<float>(tmp_value_);
			break;
		}
	case 'd':
		{
			double tmp_value_;
			s >> tmp_value_;
			value = QVariant::fromValue<double>(tmp_value_);
			break;
		}
	case 'D':
		{
			QAMQP::Frame::decimal v;
			s >> v.scale;
			s >> v.value;
			value = QVariant::fromValue<QAMQP::Frame::decimal>(v);
		}
		break;
	case 's':
		s >> nameSize_;
		tmp.resize(nameSize_);
		s.readRawData(tmp.data(), tmp.size());
		#if QT_VERSION < 0x050000
		value = QString::fromAscii(tmp.data(), nameSize_);
		#else // For Qt5
		value = QString::fromLatin1(tmp.data(), nameSize_);
		#endif
		break;
	case 'S':
		{
			quint32 length_ = 0;
			s >> length_;
			nameSize_ = length_;
			tmp.resize(length_);
		}		
		s.readRawData(tmp.data(), tmp.size());
		#if QT_VERSION < 0x050000
		value = QString::fromAscii(tmp.data(), tmp.size());
		#else // For Qt5
		value = QString::fromLatin1(tmp.data(), tmp.size());
		#endif
		break;
	case 'A':
		{
			qint32 length_ = 0;
			qint8 type = 0;
			s >> length_;
			QList<QVariant> array_;
			for (int i =0; i < length_; ++i)
			{				
				s >> type;
				array_ << readField(type, s);
			}
			value = array_;
		}
		break;
	case 'T':
		{
			qulonglong tmp_value_;
			s >> tmp_value_;			
			value = QDateTime::fromMSecsSinceEpoch(tmp_value_);
			break;
		}
	case 'F':
		{
			TableField table_;
			deserialize(s, table_);
			value = table_;
		}
		break;
	case 'V':
		break;
	default:
		qWarning("Unknown field type");
	}
	return value;
}

QDataStream & QAMQP::Frame::deserialize( QDataStream & stream, QAMQP::Frame::TableField & f )
{
	QByteArray data;	
	stream >> data;
	QDataStream s(&data, QIODevice::ReadOnly);

	while(!s.atEnd())
	{
		qint8 valueType = 0;

		QString name = readField('s', s).toString();
		s >> valueType;		
		f[name] = readField(valueType, s);
	}

	return stream;
}

QDataStream & QAMQP::Frame::serialize( QDataStream & stream, const TableField & f )
{
	QByteArray data;	
	QDataStream s(&data, QIODevice::WriteOnly);
	TableField::ConstIterator i;
	for(i = f.begin(); i != f.end(); ++i)
	{
		writeField('s', s, i.key());
		writeField(s, i.value());
	}
	if(data.isEmpty())
	{
		stream << qint32(0);
	} else {
		stream << data;
	}
	
	return stream;
}

void QAMQP::Frame::print( const TableField & f )
{
	TableField::ConstIterator i;
	for(i = f.begin(); i != f.end(); ++i)
	{
		switch(i.value().type())
		{
		case  QVariant::Hash:
			qDebug() << "\t" << qPrintable(i.key()) << ": FIELD_TABLE";
			break;
		case QVariant::List:
			qDebug() << "\t" << qPrintable(i.key()) << ": ARRAY";
			break;
		default:
			qDebug() << "\t" << qPrintable(i.key()) << ": " << i.value();
		}		
	}
}

void QAMQP::Frame::writeField( qint8 valueType, QDataStream &s, const QVariant & value, bool withType )
{
	QByteArray tmp;
	if(withType)
		s << valueType;

	switch(valueType)
	{
	case 't':
		s << (value.toBool() ? qint8(1) : qint8(0));
		break;
	case 'b':
		s << qint8(value.toInt());
		break;
	case 'B':
		s << quint8(value.toUInt());
		break;
	case 'U':
		s << qint16(value.toInt());
		break;
	case 'u':
		s << quint16(value.toUInt());
		break;
	case 'I':
		s << qint32(value.toInt());
		break;
	case 'i':
		s << quint32(value.toUInt());
		break;
	case 'L':
		s << qlonglong(value.toLongLong());
		break;
	case 'l':
		s << qulonglong(value.toULongLong());
		break;
	case 'f':
		s << value.toFloat();		
		break;
	case 'd':
		s << value.toDouble();		
		break;
	case 'D':
		{
			QAMQP::Frame::decimal v(value.value<QAMQP::Frame::decimal>());
			s << v.scale;
			s << v.value;
		}
		break;
	case 's':
		{
			QString str = value.toString();
			s << quint8(str.length());
			#if QT_VERSION < 0x050000
			s.writeRawData(str.toAscii().data(), str.length());
			#else // For Qt5
			s.writeRawData(str.toLatin1().data(), str.length());
			#endif			
		}
		break;
	case 'S':
		{
			QString str = value.toString();
			s << quint32(str.length());
			#if QT_VERSION < 0x050000
			s.writeRawData(str.toAscii().data(), str.length());
			#else // For Qt5
			s.writeRawData(str.toLatin1().data(), str.length());
			#endif
		}
		break;
	case 'A':
		{
			QList<QVariant> array_(value.toList());
			s << quint32(array_.count());
			for (int i =0; i < array_.count(); ++i)
			{				
				writeField(s, array_.at(i));
			}
		}
		break;
	case 'T':
		s << qulonglong(value.toDateTime().toMSecsSinceEpoch());
		break;
	case 'F':
		{
			TableField table_(value.toHash());
			serialize(s, table_);
		}
		break;
	case 'V':
		break;
	default:
		qWarning("Unknown field type");
	}
}

void QAMQP::Frame::writeField( QDataStream &s, const QVariant & value )
{
	char type = 0;
	switch(value.type())
	{
	case QVariant::Bool:
		type = 't';
		break;
	case QVariant::ByteArray:
		type = 'S';
		break;
	case QVariant::Int:
		{
			int i = qAbs(value.toInt());
			if(i <= qint8(0xFF)) {
				type = 'b';
			} else if(i <= qint16(0xFFFF)) {
				type = 'U';
			} else if(i <= qint16(0xFFFFFFFF)) {
				type = 'I';
			}		
		}		
		break;
	case QVariant::UInt:
		{
			int i = value.toInt();
			if(i <= qint8(0xFF)) {
				type = 'B';
			} else if(i <= qint16(0xFFFF)) {
				type = 'u';
			} else if(i <= qint16(0xFFFFFFFF)) {
				type = 'i';
			}		
		}
		break;
	case QVariant::LongLong:
		type = 'L';
		break;
	case QVariant::ULongLong:
		type = 'l';
		break;
	case QVariant::String:			
		type = 'S';
		break;
	case QVariant::DateTime:
		type = 'T';
		break;
	case QVariant::Double:
		type = value.toDouble() > FLT_MAX ? 'd' : 'f';
		break;
	case QVariant::Hash:
		type = 'F';
		break;
	case QVariant::List:
		type = 'A';
		break;
	default:;
	}

	if(type)
		writeField(type, s, value, true);
}

//////////////////////////////////////////////////////////////////////////

QAMQP::Frame::Content::Content():Base(ftHeader)
{

}

QAMQP::Frame::Content::Content( MethodClass methodClass ):Base(ftHeader)
{
	methodClass_ = methodClass;
}

QAMQP::Frame::Content::Content( QDataStream& raw ): Base(raw)
{
	readPayload(raw);
}

QAMQP::Frame::MethodClass QAMQP::Frame::Content::methodClass() const
{
	return MethodClass(methodClass_);
}

qint32 QAMQP::Frame::Content::size() const
{
	QDataStream out(&buffer_, QIODevice::WriteOnly);
	buffer_.clear();
	out << qint16(methodClass_);
	out << qint16(0); //weight
	out << qlonglong(body_.size());

	qint16 prop_ = 0;
	foreach (int p, properties_.keys())
	{
		prop_ |= p;
	}

	out << prop_;

	if(prop_ & cpContentType)
		writeField('s', out, properties_[cpContentType]);

	if(prop_ & cpContentEncoding)
		writeField('s', out, properties_[cpContentEncoding]);

	if(prop_ & cpHeaders)
		writeField('F', out, properties_[cpHeaders]);

	if(prop_ & cpDeliveryMode)
		writeField('b', out, properties_[cpDeliveryMode]);

	if(prop_ & cpPriority)
		writeField('b', out, properties_[cpPriority]);

	if(prop_ & cpCorrelationId)
		writeField('s', out, properties_[cpCorrelationId]);

	if(prop_ & cpReplyTo)
		writeField('s', out, properties_[cpReplyTo]);

	if(prop_ & cpExpiration)
		writeField('s', out, properties_[cpExpiration]);

	if(prop_ & cpMessageId)
		writeField('s', out, properties_[cpMessageId]);

	if(prop_ & cpTimestamp)
		writeField('T', out, properties_[cpTimestamp]);

	if(prop_ & cpType)
		writeField('s', out, properties_[cpType]);

	if(prop_ & cpUserId)
		writeField('s', out, properties_[cpUserId]);

	if(prop_ & cpAppId)
		writeField('s', out, properties_[cpAppId]);

	if(prop_ & cpClusterID)
		writeField('s', out, properties_[cpClusterID]);

	return buffer_.size();
}

void QAMQP::Frame::Content::setBody( const QByteArray & data )
{
	body_ = data;
}

QByteArray QAMQP::Frame::Content::body() const
{
	return body_;
}

void QAMQP::Frame::Content::setProperty( Property prop, const QVariant & value )
{
	properties_[prop] = value;
}

QVariant QAMQP::Frame::Content::property( Property prop ) const
{
	return properties_.value(prop);
}

void QAMQP::Frame::Content::writePayload( QDataStream & out ) const
{
	out.writeRawData(buffer_.data(), buffer_.size());
}

void QAMQP::Frame::Content::readPayload( QDataStream & in )
{
	in >> methodClass_;
	in.skipRawData(2); //weight
	in >> bodySize_;
	qint16 flags_ = 0;
	in >> flags_;
	if(flags_ & cpContentType)
		properties_[cpContentType] = readField('s', in);

	if(flags_ & cpContentEncoding)
		properties_[cpContentEncoding] = readField('s', in);

	if(flags_ & cpHeaders)
		properties_[cpHeaders] = readField('F', in);

	if(flags_ & cpDeliveryMode)
		properties_[cpDeliveryMode] = readField('b', in);

	if(flags_ & cpPriority)
		properties_[cpPriority] = readField('b', in);

	if(flags_ & cpCorrelationId)
		properties_[cpCorrelationId] = readField('s', in);

	if(flags_ & cpReplyTo)
		properties_[cpReplyTo] = readField('s', in);

	if(flags_ & cpExpiration)
		properties_[cpExpiration] = readField('s', in);

	if(flags_ & cpMessageId)
		properties_[cpMessageId] = readField('s', in);

	if(flags_ & cpTimestamp)
		properties_[cpTimestamp] = readField('T', in);

	if(flags_ & cpType)
		properties_[cpType] = readField('s', in);

	if(flags_ & cpUserId)
		properties_[cpUserId] = readField('s', in);

	if(flags_ & cpAppId)
		properties_[cpAppId] = readField('s', in);

	if(flags_ & cpClusterID)
		properties_[cpClusterID] = readField('s', in);
}

qlonglong QAMQP::Frame::Content::bodySize() const
{
	return body_.isEmpty() ? bodySize_ : body_.size();
}
//////////////////////////////////////////////////////////////////////////

ContentBody::ContentBody() : Base(ftBody)
{}

QAMQP::Frame::ContentBody::ContentBody( QDataStream& raw ): Base(raw)
{
	readPayload(raw);
}

void QAMQP::Frame::ContentBody::setBody( const QByteArray & data )
{
	body_ = data;
}

QByteArray QAMQP::Frame::ContentBody::body() const
{
	return body_;
}

void QAMQP::Frame::ContentBody::writePayload( QDataStream & out ) const
{
	out.writeRawData(body_.data(), body_.size());
}

void QAMQP::Frame::ContentBody::readPayload( QDataStream & in )
{
	body_.resize(size_);
	in.readRawData(body_.data(), body_.size());
}

qint32 QAMQP::Frame::ContentBody::size() const
{
	return body_.size();
}

//////////////////////////////////////////////////////////////////////////

QAMQP::Frame::Heartbeat::Heartbeat() : Base(ftHeartbeat) {}

void QAMQP::Frame::Heartbeat::readPayload(QDataStream & ) {}
void QAMQP::Frame::Heartbeat::writePayload(QDataStream & ) const {}

