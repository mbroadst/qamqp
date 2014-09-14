#include <QDateTime>
#include <QElapsedTimer>
#include <QUuid>
#include <QSignalSpy>
#include <QTimer>
#include <QDebug>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "producer.h"

Producer::Producer(const QString &id, const TestOptions &options)
    : m_options(options),
      m_id(id)
{
}

void Producer::run()
{
    QAmqpClient client;
    client.connectToHost(QHostAddress::LocalHost);
    if (!waitForSignal(&client, SIGNAL(connected()))) {
        qDebug() << "could not connect client";
        qDebug() << client.error();
        qDebug() << client.errorString();
        return;
    }

    QAmqpExchange *exchange = client.createExchange(m_options.exchangeName);
    exchange->declare(m_options.exchangeType);
    if (!waitForSignal(exchange, SIGNAL(declared()))) {
        qDebug() << "could not declare exchange";
        qDebug() << exchange->error();
        qDebug() << exchange->errorString();
        return;
    }

    QElapsedTimer timer;
    timer.start();

    qint64 startTime = 0;
    int msgCount = 0;

    while ((m_options.timeLimit == 0 || timer.elapsed() < startTime + m_options.timeLimit) &&
           (m_options.producerMsgCount == 0 || msgCount < m_options.producerMsgCount)) {
        // delay(timer.elapsed(), lastStatsTime, msgCount);

        // publish
        // Message::PropertyHash properties;
        // Exchange::PublishOption options = Exchange::poImmediate | Exchange::poMandatory;
        QByteArray message = QByteArray::number(msgCount) + QByteArray::number(QDateTime::currentMSecsSinceEpoch());
        exchange->publish(message, m_id);

//        unconfirmedSet.add(channel.getNextPublishSeqNo());
//        channel.basicPublish(exchangeName, id,
//                             mandatory, immediate,
//                             persistent ? MessageProperties.MINIMAL_PERSISTENT_BASIC : MessageProperties.MINIMAL_BASIC,
//                             msg);


//        JAVADOC
//        void basicPublish(java.lang.String exchange,
//                        java.lang.String routingKey,
//                        boolean mandatory,
//                        boolean immediate,
//                        AMQP.BasicProperties props,
//                        byte[] body)

        msgCount++;
    }
}

void Producer::delay(qint64 now, qint64 lastStatsTime, int msgCount)
{
    qint64 elapsed = now - lastStatsTime;
    //example: rateLimit is 5000 msg/s,
    //10 ms have elapsed, we have sent 200 messages
    //the 200 msgs we have actually sent should have taken us
    //200 * 1000 / 5000 = 40 ms. So we pause for 40ms - 10ms
    qint64 pause = (qint64) (m_options.rateLimit == 0.0f ?
        0.0f : (msgCount * 1000.0 / m_options.rateLimit - elapsed));

    if (pause > 0) {
      //      ::usleep(pause);
    }
}

bool Producer::waitForSignal(QObject *obj, const char *signal, int delay)
{
    QEventLoop loop;
    QObject::connect(obj, signal, &loop, SLOT(quit()), Qt::QueuedConnection);
    QSignalSpy spy(obj, signal);
    QTimer::singleShot(delay * 1000, &loop, SLOT(quit()));
    loop.exec();
    return (spy.size() == 1);
}
