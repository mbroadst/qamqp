#include <QtTest/QtTest>

#include "qamqpchannel.h"
#include "qamqpchannel_p.h"
#include "qamqpexchange_p.h"

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

    void resetState()
    {
        Q_D(QAmqpChannel);
        d->resetInternalState();
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
    void publishDeferral_data();
    void publishDeferral();
    void flowStateResetOnReconnect();
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

static QAmqpMethodFrame flowFrame(quint16 channelNumber, bool active)
{
    QAmqpMethodFrame frame(QAmqpFrame::Channel, QAmqpChannelPrivate::miFlow);
    frame.setChannel(channelNumber);

    QByteArray arguments;
    QDataStream stream(&arguments, QIODevice::WriteOnly);
    QAmqpFrame::writeAmqpField(stream, QAmqpMetaType::ShortShortUint, active ? 1 : 0);
    frame.setArguments(arguments);
    return frame;
}

void tst_QAMQPFlow::publishDeferral_data()
{
    QTest::addColumn<bool>("channelOpened");
    QTest::addColumn<bool>("flowActive");
    QTest::addColumn<bool>("deferred");

    QTest::newRow("open-and-active") << true << true << false;
    QTest::newRow("open-but-paused") << true << false << true;
    QTest::newRow("not-open-active") << false << true << true;
    QTest::newRow("not-open-paused") << false << false << true;
}

void tst_QAMQPFlow::publishDeferral()
{
    QFETCH(bool, channelOpened);
    QFETCH(bool, flowActive);
    QFETCH(bool, deferred);

    QCOMPARE(QAmqpExchangePrivate::shouldDeferPublish(channelOpened, flowActive), deferred);
}

void tst_QAMQPFlow::flowStateResetOnReconnect()
{
    TestChannel channel;
    channel.receiveMethod(flowFrame(channel.channelNumber(), false));
    QVERIFY(!channel.isFlowActive());

    // a reconnected channel starts out active again, otherwise publishes would
    // stay queued forever waiting for a channel.flow that never arrives
    channel.resetState();
    QVERIFY(channel.isFlowActive());
}

QTEST_APPLESS_MAIN(tst_QAMQPFlow)
#include "tst_qamqpflow.moc"