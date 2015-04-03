#ifndef _ISSUE23_H
#define _ISSUE23_H

/*!
 * @file Issue23.h
 * Test for possible multiple-consumers on a channel.
 * Issue 23: https://github.com/mbroadst/qamqp/issues/23
 */

#include <QByteArray>
#include <QObject>

class QAmqpClient;
class QAmqpQueue;
class QAmqpExchange;
class QTimer;

class Issue23Test : public QObject {
Q_OBJECT

public:
    Issue23Test(QAmqpClient* client, QObject* parent=0);
    ~Issue23Test();

    /*! Number of messages per consumer to send */
    static int NUM_MSGS;

    /*! Our message payload.  We should only see this once. */
    static const QByteArray MSG_PAYLOAD;

    /*! Run the test */
    void run();

signals:
    /*!
     * Signal indicating when the test is complete.  We should see
     * consumers == 1, sent == consumers*NUM_MSGS, sent == passes and
     * failures == 0.
     */
    void testComplete(int consumers, int sent,
            int passes, int failures);

private slots:
    /*!
     * From signal QAmqpQueue::declared, consume the queue
     * multiple times.
     */
    void queueDeclared();

    /*!
     * From signal QAmqpQueue::removed, indicates test is complete.
     */
    void queueRemoved();

    /*!
     * From signal QAmqpQueue::consuming, send a test message
     * to the queue.
     */
    void queueConsuming(const QString& consumer_tag);

    /*!
     * From signal QAmqpQueue::cancelled, remove the queue.
     */
    void queueCancelled(const QString& consumer_tag);

    /*!
     * from signal QAmqpQueue::messageReceived, grab the test
     * message and inspect its content.
     */
    void queueMessageReceived();

    /*!
     * from signal QTimer::timeout, disconnect or shut down.
     */
    void timerTimeout();
private:
    QAmqpClient*    client;
    QAmqpExchange*  default_ex;
    QAmqpQueue*     queue;
    QTimer*         timer;

    int             consumers;
    int             sent;
    int             passes;
    int             failures;
    bool            remove_attempted;

    /*! Try to remove the queue */
    void tryRemove();

    /*! Report the results */
    void reportResults();
};

#endif
