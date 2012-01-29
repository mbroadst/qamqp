
#include <QtCore/QCoreApplication>
#include "amqp.h"
#include "amqp_exchange.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QUrl con(QString("amqp://guest:16141614@main:5672/"));
	QAMQP::Client client(con);
	QAMQP::Exchange * exchange_ =  client.createExchange(),
		*exchange2_ =  client.createExchange();

	client.printConnect();

	return a.exec();
}
