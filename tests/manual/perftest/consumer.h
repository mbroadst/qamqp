#ifndef CONSUMER_P
#define CONSUMER_P

#include <QRunnable>
#include <QObject>

#include "testoptions.h"
#include "qamqpclient.h"

class Consumer : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit Consumer(const QString &id, const TestOptions &options);
    ~Consumer();

    virtual void run();

private:
    bool waitForSignal(QObject *obj, const char *signal, int delay = 5);
    bool exchangeExists(QAmqpClient *client, const QString &exchangeName);
    bool queueExists(QAmqpClient *client, const QString &exchangeName);

    QAmqpClient m_client;
    TestOptions m_options;
    QString m_id;

};

#endif
