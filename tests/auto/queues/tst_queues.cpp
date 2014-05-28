#include <QtTest/QtTest>

#include "amqp_client.h"
#include "amqp_queue.h"

class tst_Queues : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void declare();

};

void tst_Queues::declare()
{



}

QTEST_MAIN(tst_Queues)
#include "tst_queues.moc"
