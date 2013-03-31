QAMQP
=============
Qt4/Qt5 implementation of AMQP 0.9.1.

Implement
------------
### Connection
work with socket connections

* start - start connection negotiation
* startok - select security mechanism and locale
* tune - propose connection tuning parameters
* tuneok - negotiate connection tuning parameters
* open - open connection to virtual host
* openok - signal that connection is ready
* close - request a connection close
* closeok - confirm a connection close

### Channel
work with channels

* open - open a channel for use
* openok - signal that the channel is ready
* close - request a channel close
* closeok - confirm a channel close

### Exchange 
work with exchanges

* declare - verify exchange exists, create if needed
* declareok - confirm exchange declaration
* delete - delete an exchange
* deleteok - confirm deletion of an exchange

### Queue 
work with queues 

* declare - declare queue, create if needed
* declareok - confirms a queue definition
* bind - bind queue to an exchange
* bindok - confirm bind successful
* unbind - unbind a queue from an exchange
* unbindok - confirm unbind successful
* purge - purge a queue
* purgeok - confirms a queue purge
* delete - delete a queue
* deleteok - confirm deletion of a queue

### Basic
work with basic content 

* qos - specify quality of service
* qosok - confirm the requested qos
* consume - start a queue consumer
* consumeok - confirm a new consumer
* publish - publish a message
* deliver - notify the client of a consumer message
* get - direct access to a queue
* getok - provide client with a message
* getempty - indicate no messages available
* ack - acknowledge one or more messages

Usage
------------

    Test::Test()	
	{
		QUrl con(QString("amqp://guest:guest@localhost:5672/"));
		client_ = new QAMQP::Client(this);
		connect(client_, SIGNAL(connected()), this, SLOT(connected()));
		client_->open(con);
		exchange_ =  client_->createExchange("test.test2");
		queue_ = client_->createQueue("test.my_queue", exchange_->channelNumber());
			
		connect(queue_, SIGNAL(declared()), this, SLOT(declared()));
		connect(queue_, SIGNAL(messageRecieved()), this, SLOT(newMessage()));	

	}
	
	void Test::connected()
	{
		exchange_->declare("fanout");		
		queue_->declare();
		exchange_->bind(queue_);
	}

	void Test::declared()
	{
		exchange_->publish("Hello world", exchange_->name());
		queue_->setQOS(0,10);
		queue_->setConsumerTag("qamqp-consumer");
		queue_->consume(QAMQP::Queue::coNoAck);
	}

	void Test::newMessage()
	{
		QAMQP::Queue * q = qobject_cast<QAMQP::Queue *>(sender());
		while (q->hasMessage())
		{
			QAMQP::MessagePtr message = q->getMessage();
			qDebug("+ RECEIVE MESSAGE");
			qDebug("| Exchange-name: %s", qPrintable(message->exchangeName));
			qDebug("| Routing-key: %s", qPrintable(message->routeKey));
			qDebug("| Content-type: %s", qPrintable(message->property[QAMQP::Frame::Content::cpContentType].toString()));
			if(!q->noAck())
			{
				q->ack(message);
			}
		}
	}
Credits
================
Thank you  [@mdhooge](https://github.com/mdhooge) for tutorials inspired by https://github.com/jonnydee/nzmqt
	
[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/fda6b79d2e88186cba0c70e204c4f10b "githalytics.com")](http://githalytics.com/fuCtor/QAMQP)
