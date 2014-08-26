#ifndef amqp_channel_p_h__
#define amqp_channel_p_h__

#include <QPointer>
#include "amqp_frame_p.h"

#define METHOD_ID_ENUM(name, id) name = id, name ## Ok

namespace QAMQP
{

class Client;
class Network;
class ClientPrivate;
class ChannelPrivate : public Frame::MethodHandler
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
        METHOD_ID_ENUM(bmRecover, 110)
    };

    ChannelPrivate(Channel *q);
    virtual ~ChannelPrivate();

    void init(int channel, Client *client);
    void sendFrame(const Frame::Base &frame);

    void open();
    void flow(bool active);
    void flowOk();
    void close(int code, const QString &text, int classId, int methodId);

    // reimp MethodHandler
    virtual bool _q_method(const Frame::Method &frame);
    void openOk(const Frame::Method &frame);
    void flow(const Frame::Method &frame);
    void flowOk(const Frame::Method &frame);
    void close(const Frame::Method &frame);
    void closeOk(const Frame::Method &frame);
    void qosOk(const Frame::Method &frame);

    // private slots
    virtual void _q_disconnected();
    void _q_open();

    QPointer<Client> client;
    QString name;
    int channelNumber;
    static int nextChannelNumber;
    bool opened;
    bool needOpen;

    qint32 prefetchSize;
    qint32 requestedPrefetchSize;
    qint16 prefetchCount;
    qint16 requestedPrefetchCount;

    Error error;
    QString errorString;

    Q_DECLARE_PUBLIC(Channel)
    Channel * const q_ptr;
};

} // namespace QAMQP

#endif // amqp_channel_p_h__
