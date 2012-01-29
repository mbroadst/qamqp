/*
*  QAMQP.h
*  libqamqp
*
*  Created by Alexey Shcherbakov on 28.01.2012.
*
*/
#ifndef QAMQP_h__
#define QAMQP_h__

#define AMQPDEBUG ":5673"

#define AMQP_AUTODELETE		1
#define AMQP_DURABLE		2
#define AMQP_PASSIVE		4
#define AMQP_MANDATORY		8
#define AMQP_IMMIDIATE		16
#define AMQP_IFUNUSED		32
#define AMQP_EXCLUSIVE		64
#define AMQP_NOWAIT			128
#define AMQP_NOACK			256
#define AMQP_NOLOCAL		512
#define AMQP_MULTIPLE		1024


#define HEADER_FOOTER_SIZE 8 //  7 bytes up front, then payload, then 1 byte footer
    // max lenght (size) of frame

#include <QString>
#include <QVector>
#include <QMap>


//export AMQP;
namespace QAMQP
{
class AMQPQueue;

enum AMQPEvents_e {
	AMQP_MESSAGE, AMQP_SIGUSR, AMQP_CANCEL, AMQP_CLOSE_CHANNEL
};

class AMQPException {
	string message;
	int code;
public:
	AMQPException(string message);
	AMQPException(amqp_rpc_reply_t * res);

	string   getMessage();
	uint16_t getReplyCode();
};



class AMQPMessage {

	char * data;
	string exchange;
	string routing_key;
	uint32_t delivery_tag;
	int message_count;
	string consumer_tag;
	AMQPQueue * queue;
	map<string,string> headers;

public :
	AMQPMessage(AMQPQueue * queue);
	~AMQPMessage();

	void setMessage(const char * data);
	char * getMessage();

	void addHeader(string name, amqp_bytes_t * value);
	void addHeader(string name, uint64_t * value);
	void addHeader(string name, uint8_t * value);
	string getHeader(string name);

	void setConsumerTag( amqp_bytes_t consumer_tag);
	void setConsumerTag( string consumer_tag);
	string getConsumerTag();

	void setMessageCount(int count);
	int getMessageCount();

	void setExchange(amqp_bytes_t exchange);
	void setExchange(string exchange);
	string getExchange();

	void setRoutingKey(amqp_bytes_t routing_key);
	void setRoutingKey(string routing_key);
	string getRoutingKey();

	uint32_t getDeliveryTag();
	void setDeliveryTag(uint32_t delivery_tag);

	AMQPQueue * getQueue();

};


class AMQPBase {
protected:
	string name;
	short parms;
	amqp_connection_state_t * cnn;
	int channelNum;
	AMQPMessage * pmessage;

	short opened;

	void checkReply(amqp_rpc_reply_t * res);
	void checkClosed(amqp_rpc_reply_t * res);
	void openChannel();


public:
	~AMQPBase();
	int getChannelNum();
	void setParam(short param);
	string getName();
	void closeChannel();
	void reopen();
	void setName(const char * name);
	void setName(string name);
};

class AMQPQueue : public AMQPBase  {
protected:
	map< AMQPEvents_e, int(*)( AMQPMessage * ) > events;
	amqp_bytes_t consumer_tag;
	uint32_t delivery_tag;
	uint32_t count;
public:
	AMQPQueue(amqp_connection_state_t * cnn, int channelNum);
	AMQPQueue(amqp_connection_state_t * cnn, int channelNum, string name);

	void Declare();
	void Declare(string name);
	void Declare(string name, short parms);

	void Delete();
	void Delete(string name);

	void Purge();
	void Purge(string name);

	void Bind(string exchangeName, string key);

	void unBind(string exchangeName, string key);

	void Get();
	void Get(short param);

	void Consume();
	void Consume(short param);

	void Cancel(amqp_bytes_t consumer_tag);
	void Cancel(string consumer_tag);

	void Ack();
	void Ack(uint32_t delivery_tag);

	AMQPMessage * getMessage() {
		return pmessage;
	}

	uint32_t getCount() {
		return count;
	}

	void setConsumerTag(string consumer_tag);
	amqp_bytes_t getConsumerTag();

	void addEvent( AMQPEvents_e eventType, int (*event)(AMQPMessage*) );

	~AMQPQueue();

private:
	void sendDeclareCommand();
	void sendDeleteCommand();
	void sendPurgeCommand();
	void sendBindCommand(const char * exchange, const char * key);
	void sendUnBindCommand(const char * exchange, const char * key);
	void sendGetCommand();
	void sendConsumeCommand();
	void sendCancelCommand();
	void sendAckCommand();
	void setHeaders(amqp_basic_properties_t * p);
};


class AMQPExchange : public AMQPBase {
	string type;
	map<string,string> sHeaders;
	map<string, int> iHeaders;

public:
	AMQPExchange(amqp_connection_state_t * cnn, int channelNum);
	AMQPExchange(amqp_connection_state_t * cnn, int channelNum, string name);

	void Declare();
	void Declare(string name);
	void Declare(string name, string type);
	void Declare(string name, string type, short parms);

	void Delete();
	void Delete(string name);

	void Bind(string queueName);
	void Bind(string queueName, string key);

	void Publish(string message, string key);

	void setHeader(string name, int value);
	void setHeader(string name, string value);

private:
	AMQPExchange();
	void checkType();
	void sendDeclareCommand();
	void sendDeleteCommand();
	void sendPublishCommand();

	void sendBindCommand(const char * queueName, const char * key);
	void sendPublishCommand(const char * message, const char * key);
	void sendCommand();
	void checkReply(amqp_rpc_reply_t * res);
	void checkClosed(amqp_rpc_reply_t * res);

};
}
#endif // QAMQP_h__
