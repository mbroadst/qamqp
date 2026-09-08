#include <QtTest/QtTest>
#include <QRandomGenerator>

#include "qamqpframe_p.h"
#include "qamqptable.h"
#include "qamqpglobal.h"

// Fuzz/boundary tests for the untrusted-input parsing code in qamqpframe.cpp and
// qamqptable.cpp. These do not require a running broker: they exercise the parsers
// directly with crafted and randomized byte buffers, asserting only that the code
// never crashes/hangs/over-allocates on malformed input.
class tst_QAMQPParser : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shortStringBoundaries_data();
    void shortStringBoundaries();

    void fieldSizeBoundaries_data();
    void fieldSizeBoundaries();

    void utf8StringRoundTrip_data();
    void utf8StringRoundTrip();

    void truncatedMethodFrame();
    void deeplyNestedArray();

    void fuzzReadAmqpField();
    void fuzzTableFieldValues();
    void fuzzTableDecode();
    void fuzzMethodFrame();

private:
    static QByteArray randomBytes(QRandomGenerator &rng, int len);
};

QByteArray tst_QAMQPParser::randomBytes(QRandomGenerator &rng, int len)
{
    QByteArray buffer;
    buffer.resize(len);
    for (int i = 0; i < len; ++i)
        buffer[i] = char(rng.bounded(256));
    return buffer;
}

void tst_QAMQPParser::shortStringBoundaries_data()
{
    QTest::addColumn<int>("declaredSize");

    QTest::addColumn<bool>("truncate");

    QTest::newRow("empty") << 0 << false;
    QTest::newRow("ascii-127") << 127 << false;
    QTest::newRow("truncated-payload") << 200 << true;
}

void tst_QAMQPParser::shortStringBoundaries()
{
    QFETCH(int, declaredSize);
    QFETCH(bool, truncate);

    const int actualBytes = truncate ? declaredSize / 4 : declaredSize;

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << quint8(declaredSize);
    out.writeRawData(QByteArray(actualBytes, 'x').constData(), actualBytes);

    QDataStream in(buffer);
    QVariant result = QAmqpFrame::readAmqpField(in, QAmqpMetaType::ShortString);

    if (!truncate) {
        QCOMPARE(result.toString().length(), declaredSize);
    } else {
        // must be rejected cleanly rather than reading garbage/out-of-bounds data
        QVERIFY(!result.isValid() || result.toString().isEmpty());
    }
}

void tst_QAMQPParser::fieldSizeBoundaries_data()
{
    QTest::addColumn<quint32>("declaredSize");
    QTest::addColumn<int>("providedBytes");
    QTest::addColumn<bool>("shouldBeRejected");

    QTest::newRow("zero") << quint32(0) << 0 << false;
    QTest::newRow("small-exact") << quint32(10) << 10 << false;
    QTest::newRow("huge") << quint32(0xFFFFFFF0u) << 16 << true;
    QTest::newRow("frame-max-plus-one") << quint32(AMQP_FRAME_MAX + 1) << 16 << true;
    QTest::newRow("declared-exceeds-available") << quint32(10000) << 16 << true;
}

void tst_QAMQPParser::fieldSizeBoundaries()
{
    QFETCH(quint32, declaredSize);
    QFETCH(int, providedBytes);
    QFETCH(bool, shouldBeRejected);

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << declaredSize;
    out.writeRawData(QByteArray(providedBytes, 'y').constData(), providedBytes);

    QDataStream in(buffer);
    QVariant result = QAmqpFrame::readAmqpField(in, QAmqpMetaType::LongString);

    if (shouldBeRejected) {
        QVERIFY(!result.isValid() || result.toString().isEmpty());
    } else {
        QCOMPARE(quint32(result.toString().toUtf8().size()), declaredSize);
    }
}

void tst_QAMQPParser::utf8StringRoundTrip_data()
{
    QTest::addColumn<QString>("text");

    // non-ASCII rows are built from explicit UTF-8 bytes so the test does not
    // depend on the encoding of this source file
    QTest::newRow("ascii") << QString::fromUtf8("plain-ascii");
    // split literal keeps the hex escape from greedily consuming the following 'e'
    QTest::newRow("latin1-supplement") << QString::fromUtf8("Gr\xC3\xBC\xC3\x9F" "e");
    QTest::newRow("cyrillic") << QString::fromUtf8("\xD0\xBE\xD1\x87\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD1\x8C");
    QTest::newRow("cjk") << QString::fromUtf8("\xE9\x98\x9F\xE5\x88\x97");
    QTest::newRow("non-bmp") << QString::fromUtf8("queue \xF0\x9F\x9A\x80");
}

void tst_QAMQPParser::utf8StringRoundTrip()
{
    QFETCH(QString, text);

    const QVector<QAmqpMetaType::ValueType> types = {
        QAmqpMetaType::ShortString, QAmqpMetaType::LongString
    };

    foreach (QAmqpMetaType::ValueType type, types) {
        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        QAmqpFrame::writeAmqpField(out, type, text);

        QDataStream in(buffer);
        QCOMPARE(QAmqpFrame::readAmqpField(in, type).toString(), text);
    }
}

void tst_QAMQPParser::truncatedMethodFrame()
{
    // a Method frame body must contain at least methodClass_+id_ (4 bytes); a
    // declared size smaller than that used to underflow arguments_.resize()
    for (qint32 size = 0; size < 4; ++size) {
        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << qint8(QAmqpFrame::Method);
        out << quint16(1);
        out << size;
        QByteArray body(size, char(0));
        out.writeRawData(body.constData(), body.size());

        QDataStream in(buffer);
        QAmqpMethodFrame frame;
        in >> frame; // must not crash
        QVERIFY(frame.arguments().isEmpty());
    }
}

void tst_QAMQPParser::deeplyNestedArray()
{
    // nested "array containing an array containing an array ... containing Void";
    // guards against unbounded recursion / stack overflow while decoding tables
    const int depth = 2000;

    QByteArray current;
    {
        QDataStream s(&current, QIODevice::WriteOnly);
        s << qint8('V');
    }

    for (int i = 0; i < depth; ++i) {
        QByteArray next;
        QDataStream s(&next, QIODevice::WriteOnly);
        s << qint8('A');
        s << quint32(current.size());
        s.writeRawData(current.constData(), current.size());
        current = next;
    }

    QByteArray encoded;
    QDataStream out(&encoded, QIODevice::WriteOnly);
    out << quint32(current.size());
    out.writeRawData(current.constData(), current.size());

    QDataStream in(encoded);
    QAmqpTable::readFieldValue(in, QAmqpMetaType::Array); // must not crash
    QCOMPARE(in.status(), QDataStream::ReadCorruptData);
}

void tst_QAMQPParser::fuzzReadAmqpField()
{
    const QVector<QAmqpMetaType::ValueType> types = {
        QAmqpMetaType::Boolean, QAmqpMetaType::ShortShortUint, QAmqpMetaType::ShortUint,
        QAmqpMetaType::LongUint, QAmqpMetaType::LongLongUint, QAmqpMetaType::ShortString,
        QAmqpMetaType::LongString, QAmqpMetaType::Timestamp, QAmqpMetaType::Hash
    };

    QRandomGenerator rng(0xC0FFEE);
    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        const QByteArray buffer = randomBytes(rng, rng.bounded(0, 300));

        for (QAmqpMetaType::ValueType type : types) {
            QDataStream in(buffer);
            QVariant result = QAmqpFrame::readAmqpField(in, type);
            if (result.canConvert<QString>())
                QVERIFY(result.toString().toUtf8().size() <= AMQP_FRAME_MAX);
        }
    }
}

void tst_QAMQPParser::fuzzTableFieldValues()
{
    const QVector<QAmqpMetaType::ValueType> types = {
        QAmqpMetaType::ShortShortInt, QAmqpMetaType::ShortInt, QAmqpMetaType::LongInt,
        QAmqpMetaType::LongLongInt, QAmqpMetaType::Float, QAmqpMetaType::Double,
        QAmqpMetaType::Decimal, QAmqpMetaType::Array, QAmqpMetaType::Bytes,
        QAmqpMetaType::Void, QAmqpMetaType::Hash, QAmqpMetaType::LongString
    };

    QRandomGenerator rng(0xBADC0DE);
    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        const QByteArray buffer = randomBytes(rng, rng.bounded(0, 400));

        for (QAmqpMetaType::ValueType type : types) {
            QDataStream in(buffer);
            QAmqpTable::readFieldValue(in, type); // must not crash/hang
        }
    }
}

void tst_QAMQPParser::fuzzTableDecode()
{
    QRandomGenerator rng(0x5EEDBEEF);
    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        const QByteArray buffer = randomBytes(rng, rng.bounded(0, 500));

        QDataStream in(buffer);
        QAmqpTable table;
        in >> table; // must not crash/hang
    }
}

void tst_QAMQPParser::fuzzMethodFrame()
{
    QRandomGenerator rng(0xFEEDFACE);
    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        // deliberately mismatch the declared size against the bytes actually written,
        // exercising both over- and under-declared frame sizes
        const qint32 declaredSize = rng.bounded(0, 400);
        const int actualBytes = rng.bounded(0, 400);

        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << qint8(QAmqpFrame::Method);
        out << quint16(1);
        out << declaredSize;
        const QByteArray body = randomBytes(rng, actualBytes);
        out.writeRawData(body.constData(), body.size());

        QDataStream in(buffer);
        QAmqpMethodFrame frame;
        in >> frame; // must not crash/hang regardless of size/actualBytes mismatch
    }
}

QTEST_APPLESS_MAIN(tst_QAMQPParser)
#include "tst_qamqpparser.moc"
