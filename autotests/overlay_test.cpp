/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include "previewproducer/overlay.h"

class OverlayTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void circleWrapsTheFaceRectangle()
    {
        const Overlay::Face face = Overlay::faceCircle(100, 50, 200, 150);
        QCOMPARE(face.x, 150.0);
        QCOMPARE(face.y, 100.0);
        // half the width, 20 % larger, like Howdy's own preview
        QCOMPARE(face.r, 60.0);
        QCOMPARE(face.match, Overlay::Match::Unchecked);
    }

    void classifyMatch_data()
    {
        QTest::addColumn<int>("index");
        QTest::addColumn<double>("distance");
        QTest::addColumn<double>("certainty");
        QTest::addColumn<Overlay::Match>("expected");

        QTest::newRow("no such model") << -1 << 0.1 << 3.5 << Overlay::Match::Failed;
        QTest::newRow("below the limit") << 0 << 0.30 << 3.5 << Overlay::Match::Matched;
        QTest::newRow("on the limit") << 1 << 0.40 << 4.0 << Overlay::Match::NoMatch;
        QTest::newRow("above the limit") << 2 << 0.50 << 3.5 << Overlay::Match::NoMatch;
    }

    void classifyMatch()
    {
        QFETCH(int, index);
        QFETCH(double, distance);
        QFETCH(double, certainty);
        QFETCH(Overlay::Match, expected);

        QCOMPARE(Overlay::classify(index, distance, certainty), expected);
    }

    void toVariantCarriesWhatQmlDraws()
    {
        Overlay::Info info;
        info.frameWidth = 640;
        info.frameHeight = 360;
        info.dark = false;
        info.hist = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 72.f };
        Overlay::Face face = Overlay::faceCircle(100, 50, 200, 150);
        face.match = Overlay::Match::Matched;
        face.model = 2;
        face.certainty = 2.5;
        info.faces = { face };
        info.fps = 15;
        info.frames = 120;
        info.recognitionMs = 42;

        const QVariantMap map = Overlay::toVariant(info);

        QCOMPARE(map.value(QStringLiteral("frameWidth")).toInt(), 640);
        QCOMPARE(map.value(QStringLiteral("frameHeight")).toInt(), 360);
        QCOMPARE(map.value(QStringLiteral("dark")).toBool(), false);
        QCOMPARE(map.value(QStringLiteral("fps")).toInt(), 15);
        QCOMPARE(map.value(QStringLiteral("frames")).toInt(), 120);
        QCOMPARE(map.value(QStringLiteral("recognitionMs")).toInt(), 42);

        const QVariantList hist = map.value(QStringLiteral("hist")).toList();
        QCOMPARE(hist.size(), 8);
        QCOMPARE(hist.at(7).toDouble(), 72.0);

        const QVariantList faces = map.value(QStringLiteral("faces")).toList();
        QCOMPARE(faces.size(), 1);
        const QVariantMap f = faces.at(0).toMap();
        QCOMPARE(f.value(QStringLiteral("x")).toDouble(), 150.0);
        QCOMPARE(f.value(QStringLiteral("y")).toDouble(), 100.0);
        QCOMPARE(f.value(QStringLiteral("r")).toDouble(), 60.0);
        QCOMPARE(f.value(QStringLiteral("state")).toString(), QStringLiteral("matched"));
        QCOMPARE(f.value(QStringLiteral("model")).toInt(), 2);
        QCOMPARE(f.value(QStringLiteral("certainty")).toDouble(), 2.5);
    }

    void stateNames_data()
    {
        QTest::addColumn<Overlay::Match>("match");
        QTest::addColumn<QString>("name");

        QTest::newRow("unchecked") << Overlay::Match::Unchecked << QStringLiteral("unchecked");
        QTest::newRow("failed") << Overlay::Match::Failed << QStringLiteral("failed");
        QTest::newRow("matched") << Overlay::Match::Matched << QStringLiteral("matched");
        QTest::newRow("no match") << Overlay::Match::NoMatch << QStringLiteral("nomatch");
    }

    void stateNames()
    {
        QFETCH(Overlay::Match, match);
        QFETCH(QString, name);

        Overlay::Info info;
        Overlay::Face face;
        face.match = match;
        info.faces = { face };

        const QVariantMap f = Overlay::toVariant(info).value(QStringLiteral("faces")).toList().at(0).toMap();
        QCOMPARE(f.value(QStringLiteral("state")).toString(), name);
    }
};

QTEST_GUILESS_MAIN(OverlayTest)

#include "overlay_test.moc"
