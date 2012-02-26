QAMQP
=============
Qt4 implementation of AMQP 0.9.1.

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
