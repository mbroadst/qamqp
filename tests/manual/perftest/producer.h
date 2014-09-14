#ifndef PRODUCER_H
#define PRODUCER_H

#include <QRunnable>
#include "testoptions.h"

class Producer : public QRunnable
{
public:
    Producer(const QString &id, const TestOptions &options);

    virtual void run();

private:
    bool waitForSignal(QObject *obj, const char *signal, int delay = 5);
    void delay(qint64 now, qint64 lastStatsTime, int msgCount);

    TestOptions m_options;
    QString m_id;

};

#endif
