#ifndef QAMQPCHANNEL_P_H
#define QAMQPCHANNEL_P_H

#include <QPointer>
#include "qamqpframe_p.h"

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

    enum ChannelState {
        /*! Channel is presently closed */
        ChannelClosedState,
        /*! Channel is being opened, pending response. */
        ChannelOpeningState,
        /*! Defining QOS parameters for channel. */
        SetQOSState,
        /*! Channel is open */
        ChannelOpenState,
        /*! Channel is being closed, pending response. */
        ChannelClosingState,
    };

    QAmqpChannelPrivate(QAmqpChannel *q);
    virtual ~QAmqpChannelPrivate();

    void init(int channel, QAmqpClient *client);
    void sendFrame(const QAmqpFrame &frame);

    void open();
    void flow(bool active);
    void flowOk();
    void close(int code, const QString &text, int classId, int methodId);

    // reimp MethodHandler
    virtual bool _q_method(const QAmqpMethodFrame &frame);
    void openOk(const QAmqpMethodFrame &frame);
    void flow(const QAmqpMethodFrame &frame);
    void flowOk(const QAmqpMethodFrame &frame);
    void close(const QAmqpMethodFrame &frame);
    void closeOk(const QAmqpMethodFrame &frame);
    void qosOk(const QAmqpMethodFrame &frame);

    /*!
     * Declare the channel as opened, signal listeners and call the subclass
     * channelOpened method.
     */
    void markOpened();

    // private slots
    virtual void _q_disconnected();
    void _q_open();

    QPointer<QAmqpClient> client;
    QString name;
    quint16 channelNumber;
    static quint16 nextChannelNumber;
    ChannelState channelState;
    bool needOpen;

    /*! Report and change state. */
    virtual void newState(ChannelState state);

    /*! Flag: the user has defined QOS settings */
    bool qosDefined;
    /*! Flag: channel was open at time of qos call */
    bool qosWasOpen;
    qint32 prefetchSize;
    qint32 requestedPrefetchSize;
    qint16 prefetchCount;
    qint16 requestedPrefetchCount;

    QAMQP::Error error;
    QString errorString;

    Q_DECLARE_PUBLIC(QAmqpChannel)
    QAmqpChannel * const q_ptr;
};

QDebug operator<<(QDebug dbg, QAmqpChannelPrivate::ChannelState s);

#endif // QAMQPCHANNEL_P_H
