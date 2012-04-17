#include <QObject>
#include "qamqp/amqp.h"
#include "qamqp/amqp_exchange.h"
#include "qamqp/amqp_queue.h"
#include <QPointer>

class Test : public QObject
{
	Q_OBJECT
	
public:
	Test();
	~Test();

	Q_INVOKABLE void test() {};
private slots:

	void declared();
	void newMessage();

private:
	QPointer<QAMQP::Client> client_;
	QPointer<QAMQP::Exchange> exchange_;
	QPointer<QAMQP::Queue> queue_, queue2_;
};