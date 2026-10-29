/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include "previewproducer/recognitionpacer.h"

class RecognitionPacerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void firstFrameIsScanned()
    {
        RecognitionPacer pacer;
        QVERIFY(pacer.due(0));
    }

    void waitsTwiceTheLastRecognition()
    {
        RecognitionPacer pacer;
        // a recognition from 1000 to 1150 ms took 150 ms
        pacer.done(1000, 1150);
        QVERIFY(!pacer.due(1151));
        QVERIFY(!pacer.due(1449));
        // 1150 + 2 * 150
        QVERIFY(pacer.due(1450));
        QVERIFY(pacer.due(2000));
    }

    void followsTheLatestDuration()
    {
        RecognitionPacer pacer;
        pacer.done(1000, 1300);
        pacer.done(2000, 2050);
        // only the last one counts: 2050 + 2 * 50
        QVERIFY(!pacer.due(2149));
        QVERIFY(pacer.due(2150));
    }

    void instantRecognitionDoesNotWait()
    {
        RecognitionPacer pacer;
        pacer.done(500, 500);
        QVERIFY(pacer.due(500));
    }
};

QTEST_GUILESS_MAIN(RecognitionPacerTest)

#include "recognitionpacer_test.moc"
