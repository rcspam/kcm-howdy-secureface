/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include "usersfilter.h"

class UsersFilterTest : public QObject
{
    Q_OBJECT

private:
    const QStringList m_shells{ QStringLiteral("/bin/sh"), QStringLiteral("/bin/bash"), QStringLiteral("/usr/bin/zsh") };
    const UsersFilter::UidRange m_range{ 1000, 60000 };

private Q_SLOTS:
    void parseLoginDefs_data()
    {
        QTest::addColumn<QString>("content");
        QTest::addColumn<unsigned>("min");
        QTest::addColumn<unsigned>("max");

        QTest::newRow("ubuntu") << QStringLiteral("# comment\nUID_MIN\t\t\t 1000\nUID_MAX\t\t\t60000\nSYS_UID_MIN\t\t  100\n") << 1000u << 60000u;
        QTest::newRow("custom") << QStringLiteral("UID_MIN 500\nUID_MAX 30000\n") << 500u << 30000u;
        QTest::newRow("commented out") << QStringLiteral("#UID_MIN 500\n# UID_MAX 30000\n") << 1000u << 60000u;
        QTest::newRow("missing") << QString() << 1000u << 60000u;
        QTest::newRow("garbage value") << QStringLiteral("UID_MIN abc\nUID_MAX 42000\n") << 1000u << 42000u;
        QTest::newRow("SYS_ keys ignored") << QStringLiteral("SYS_UID_MIN 100\nSYS_UID_MAX 999\n") << 1000u << 60000u;
    }

    void parseLoginDefs()
    {
        QFETCH(QString, content);
        QFETCH(unsigned, min);
        QFETCH(unsigned, max);

        const auto range = UsersFilter::parseLoginDefs(content);
        QCOMPARE(range.min, min);
        QCOMPARE(range.max, max);
    }

    void isRealUser_data()
    {
        QTest::addColumn<unsigned>("uid");
        QTest::addColumn<QString>("shell");
        QTest::addColumn<bool>("expected");

        QTest::newRow("first user") << 1000u << QStringLiteral("/bin/bash") << true;
        QTest::newRow("second user, zsh") << 1003u << QStringLiteral("/usr/bin/zsh") << true;
        QTest::newRow("empty shell means /bin/sh") << 1002u << QString() << true;
        QTest::newRow("system user with a real shell") << 988u << QStringLiteral("/bin/sh") << false;
        QTest::newRow("system user") << 999u << QStringLiteral("/bin/false") << false;
        QTest::newRow("nologin above UID_MAX") << 64055u << QStringLiteral("/usr/sbin/nologin") << false;
        QTest::newRow("nobody") << 65534u << QStringLiteral("/usr/sbin/nologin") << false;
        QTest::newRow("regular uid, nologin") << 1001u << QStringLiteral("/usr/sbin/nologin") << false;
        QTest::newRow("above UID_MAX") << 60001u << QStringLiteral("/bin/bash") << false;
        QTest::newRow("shell not in /etc/shells") << 1004u << QStringLiteral("/opt/kiosk/run") << false;
    }

    void isRealUser()
    {
        QFETCH(unsigned, uid);
        QFETCH(QString, shell);
        QFETCH(bool, expected);

        QCOMPARE(UsersFilter::isRealUser(uid, shell, m_range, m_shells), expected);
    }

    void nologinListedInShellsIsStillRejected()
    {
        const QStringList shells{ QStringLiteral("/bin/bash"), QStringLiteral("/usr/sbin/nologin"), QStringLiteral("/bin/false") };
        QVERIFY(!UsersFilter::isRealUser(1001, QStringLiteral("/usr/sbin/nologin"), m_range, shells));
        QVERIFY(!UsersFilter::isRealUser(1001, QStringLiteral("/bin/false"), m_range, shells));
    }
};

QTEST_GUILESS_MAIN(UsersFilterTest)

#include "usersfilter_test.moc"
