#ifndef QAMQPCHANNEL_P_H
#define QAMQPCHANNEL_P_H

#include <QPointer>
#include "qamqpframe_p.h"
#include "qamqptable.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

class QAmqpChannel;
class QAmqpClient;
class QAmqpClientPrivate;
class QAmqpChannelPrivate : public QAmqpMethodFrameHandler
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miOpen, 10),
        METHOD_ID_ENUM(miFlow, 20),
        METHOD_ID_ENUM(miClose, 40)
    };

    enum BasicMethod {
        METHOD_ID_ENUM(bmQos, 10),
        METHOD_ID_ENUM(bmConsume, 20),
        METHOD_ID_ENUM(bmCancel, 30),
        bmPublish = 40,
        bmReturn = 50,
        bmDeliver = 60,
        METHOD_ID_ENUM(bmGet, 70),
        bmGetEmpty = 72,
        bmAck = 80,
        bmReject = 90,
        bmRecoverAsync = 100,
        METHOD_ID_ENUM(bmRecover, 110),
        bmNack = 120
    };

    QAmqpChannelPrivate(QAmqpChannel *q);
    virtual ~QAmqpChannelPrivate();

    void init(int channel, QAmqpClient *client);
    void sendFrame(const QAmqpFrame &frame);
    virtual void resetInternalState();

    void open();
    void flow(bool active);
    void flowOk();
    void close(int code, const QString &text, int classId, int methodId);
    void notifyClosed();

    // reimp MethodHandler
    virtual bool _q_method(const QAmqpMethodFrame &frame);
    void openOk(const QAmqpMethodFrame &frame);
    void flow(const QAmqpMethodFrame &frame);
    void flowOk(const QAmqpMethodFrame &frame);
    void close(const QAmqpMethodFrame &frame);
    void closeOk(const QAmqpMethodFrame &frame);
    void qosOk(const QAmqpMethodFrame &frame);

    // private slots
    virtual void _q_disconnected();
    void _q_open();

    QPointer<QAmqpClient> client;
    QString name;
    quint16 channelNumber;
    static quint16 nextChannelNumber;
    bool opened;
    bool needOpen;

    qint32 prefetchSize;
    qint32 requestedPrefetchSize;
    qint16 prefetchCount;
    qint16 requestedPrefetchCount;

    QAMQP::Error error;
    QString errorString;

    Q_DECLARE_PUBLIC(QAmqpChannel)
    QAmqpChannel * const q_ptr;
    QAmqpTable arguments;
};

#endif // QAMQPCHANNEL_P_H
