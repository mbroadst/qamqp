#include <QtTest/QtTest>

#include "qamqpchannel.h"
#include "qamqpchannel_p.h"

class TestChannel : public QAmqpChannel
{
    Q_OBJECT
public:
    TestChannel()
        : QAmqpChannel(new QAmqpChannelPrivate(this), 0)
    {
        Q_D(QAmqpChannel);
        d->channelNumber = 1;
    }

    void receiveMethod(const QAmqpMethodFrame &frame)
    {
        Q_D(QAmqpChannel);
        d->_q_method(frame);
    }

protected:
    virtual void channelOpened() {}
    virtual void channelClosed() {}
};

class tst_QAMQPFlow : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void serverInitiatedFlow();
};

void tst_QAMQPFlow::serverInitiatedFlow()
{
    TestChannel channel;
    QSignalSpy paused(&channel, SIGNAL(paused()));
    QSignalSpy resumed(&channel, SIGNAL(resumed()));

    QAmqpMethodFrame pause(QAmqpFrame::Channel, QAmqpChannelPrivate::miFlow);
    pause.setChannel(channel.channelNumber());
    QByteArray pauseArguments;
    QDataStream pauseStream(&pauseArguments, QIODevice::WriteOnly);
    QAmqpFrame::writeAmqpField(pauseStream, QAmqpMetaType::ShortShortUint, 0);
    pause.setArguments(pauseArguments);
    channel.receiveMethod(pause);

    QVERIFY(!channel.isFlowActive());
    QCOMPARE(paused.count(), 1);
    QCOMPARE(resumed.count(), 0);

    QAmqpMethodFrame acknowledgement = QAmqpChannelPrivate::flowOkFrame(channel.channelNumber(), false);
    QCOMPARE(acknowledgement.methodClass(), QAmqpFrame::Channel);
    QCOMPARE(acknowledgement.id(), qint16(QAmqpChannelPrivate::miFlowOk));
    QCOMPARE(acknowledgement.channel(), quint16(channel.channelNumber()));
    QByteArray acknowledgementArguments = acknowledgement.arguments();
    QDataStream acknowledgementStream(&acknowledgementArguments, QIODevice::ReadOnly);
    QCOMPARE(QAmqpFrame::readAmqpField(acknowledgementStream, QAmqpMetaType::Boolean).toBool(), false);

    QAmqpMethodFrame resume(QAmqpFrame::Channel, QAmqpChannelPrivate::miFlow);
    resume.setChannel(channel.channelNumber());
    QByteArray resumeArguments;
    QDataStream resumeStream(&resumeArguments, QIODevice::WriteOnly);
    QAmqpFrame::writeAmqpField(resumeStream, QAmqpMetaType::ShortShortUint, 1);
    resume.setArguments(resumeArguments);
    channel.receiveMethod(resume);

    QVERIFY(channel.isFlowActive());
    QCOMPARE(paused.count(), 1);
    QCOMPARE(resumed.count(), 1);
}

QTEST_APPLESS_MAIN(tst_QAMQPFlow)
#include "tst_qamqpflow.moc"