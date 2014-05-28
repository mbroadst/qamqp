#include "amqp_message.h"

using namespace QAMQP;

Message::Message()
{
    qDebug("Message create");
    leftSize = 0;
    deliveryTag = 0;
}

Message::~Message()
{
    qDebug("Message release");
}
