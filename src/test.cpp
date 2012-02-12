#include "test.h"

Test::Test()
{
	QUrl con(QString("amqp://guest:guest@localhost:5672/"));
	client_ = new QAMQP::Client(this);
	client_->open(con);
	exchange_ =  client_->createExchange("test.test");
	exchange_->declare("direct");

	queue_ = client_->createQueue("test.my_queue", exchange_->channelNumber());
	queue_->declare();

	exchange_->bind(queue_);

	connect(queue_, SIGNAL(declared()), this, SLOT(declared()));
}

Test::~Test()
{

}

void Test::declared()
{
	qDebug("\t-= Ready =-");
	exchange_->publish("test 3432 432 24 23 423 32 23 4324 32 423 423 423", exchange_->name());
}