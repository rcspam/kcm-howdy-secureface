/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include "stamprules.h"

class StampRulesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readsHowdysDefaultLayout()
    {
        const QString ini = QStringLiteral(
            "[core]\n"
            "workaround = off\n"
            "\n"
            "[rubberstamps]\n"
            "# Enable specific extra checks after the user has been recognised\n"
            "enabled = false\n"
            "\n"
            "# What type of stamps to run and with what options.\n"
            "#  stamptype  timeout  (failsafe | faildeadly)   [extra_argument=value]\n"
            "stamp_rules =\n"
            "\tnod\t\t5s\t\tfailsafe     min_distance=12\n"
            "\n"
            "[debug]\n"
            "end_report = false\n");
        const QStringList rules = StampRules::parse(ini);
        QCOMPARE(rules, QStringList{QStringLiteral("nod\t\t5s\t\tfailsafe     min_distance=12")});
        QCOMPARE(StampRules::nodTimeout(rules), 5.0);
    }

    void readsSingleLineValues()
    {
        QCOMPARE(StampRules::parse(QStringLiteral("[rubberstamps]\nstamp_rules = nod 3s failsafe\n")),
                 QStringList{QStringLiteral("nod 3s failsafe")});
        // configparser also takes ":" and any case for the key
        QCOMPARE(StampRules::parse(QStringLiteral("[rubberstamps]\nStamp_Rules: nod 3s failsafe\n")),
                 QStringList{QStringLiteral("nod 3s failsafe")});
    }

    void skipsCommentsAndBlankLinesInsideTheValue()
    {
        const QString ini = QStringLiteral(
            "[rubberstamps]\n"
            "stamp_rules =\n"
            "\tnod 5s failsafe\n"
            "\t# hotkey 5s faildeadly\n"
            "\n"
            "\thotkey 3s failsafe\n");
        QCOMPARE(StampRules::parse(ini), (QStringList{QStringLiteral("nod 5s failsafe"), QStringLiteral("hotkey 3s failsafe")}));
    }

    void stopsAtTheNextKeyOrSection()
    {
        QCOMPARE(StampRules::parse(QStringLiteral("[rubberstamps]\nstamp_rules =\n\tnod 5s failsafe\nother = 1\n\thotkey 3s failsafe\n")),
                 QStringList{QStringLiteral("nod 5s failsafe")});
        QCOMPARE(StampRules::parse(QStringLiteral("[rubberstamps]\nstamp_rules =\n\tnod 5s failsafe\n[debug]\n\thotkey 3s failsafe\n")),
                 QStringList{QStringLiteral("nod 5s failsafe")});
    }

    void ignoresOtherSections()
    {
        QVERIFY(StampRules::parse(QStringLiteral("[debug]\nstamp_rules = nod 5s failsafe\n")).isEmpty());
        QVERIFY(StampRules::parse(QStringLiteral("[rubberstamps]\nenabled = true\n")).isEmpty());
        QVERIFY(StampRules::parse(QString()).isEmpty());
    }

    void acceptsOneFailsafeNod()
    {
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s failsafe")}), 5.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 2.5s failsafe")}), 2.5);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 4 failsafe min_distance=12 min_directions=2")}), 4.0);
    }

    void rejectsAnythingElse()
    {
        // no rule, more than one rule, another stamp
        QCOMPARE(StampRules::nodTimeout({}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s failsafe"), QStringLiteral("nod 5s failsafe")}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("hotkey 5s failsafe")}), -1.0);
        // a nod that lets the user in when it times out
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s faildeadly")}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s FAILDEADLY")}), -1.0);
        // options the nod stamp does not know, or values it cannot read, abort every unlock
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s failsafe speed=2")}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s failsafe min_distance=1.5")}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 5s failsafe # comment")}), -1.0);
        QCOMPARE(StampRules::nodTimeout({QStringLiteral("nod 0s failsafe")}), -1.0);
    }
};

QTEST_GUILESS_MAIN(StampRulesTest)

#include "stamprules_test.moc"
