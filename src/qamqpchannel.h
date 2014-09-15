#ifndef QAMQPCHANNEL_H
#define QAMQPCHANNEL_H

#include <QObject>
#include "qamqpglobal.h"

class QAmqpClient;
class QAmqpChannelPrivate;
class QAMQP_EXPORT QAmqpChannel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int number READ channelNumber CONSTANT)
    Q_PROPERTY(bool opened READ isOpened CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName)
public:
    virtual ~QAmqpChannel();

    int channelNumber() const;
    bool isOpened() const;

    QString name() const;
    void setName(const QString &name);

    QAMQP::Error error() const;
    QString errorString() const;

    qint32 prefetchSize() const;
    qint16 prefetchCount() const;

    // AMQP Basic
    void qos(qint16 prefetchCount, qint32 prefetchSize = 0);

public Q_SLOTS:
    void close();
    void reopen();
    void resume();

Q_SIGNALS:
    void opened();
    void closed();
    void resumed();
    void paused();
    void error(QAMQP::Error error);
    void qosDefined();

protected:
    virtual void channelOpened() = 0;
    virtual void channelClosed() = 0;

protected:
    explicit QAmqpChannel(QAmqpChannelPrivate *dd, QAmqpClient *client);

    Q_DISABLE_COPY(QAmqpChannel)
    Q_DECLARE_PRIVATE(QAmqpChannel)
    QScopedPointer<QAmqpChannelPrivate> d_ptr;

    Q_PRIVATE_SLOT(d_func(), void _q_open())
    Q_PRIVATE_SLOT(d_func(), void _q_disconnected())

    friend class QAmqpClientPrivate;
};

#endif  // QAMQPCHANNEL_H
