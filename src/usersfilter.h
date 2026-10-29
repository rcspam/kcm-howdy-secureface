/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Tells the accounts people log into apart from system and service accounts,
// the same way useradd does: an UID inside UID_MIN..UID_MAX of login.defs and
// a login shell listed in /etc/shells.
namespace UsersFilter
{

struct UidRange {
    unsigned min = 1000;
    unsigned max = 60000;
};

// Reads UID_MIN and UID_MAX from the content of /etc/login.defs; a missing or
// invalid value keeps its default.
inline UidRange parseLoginDefs(const QString &content)
{
    UidRange range;
    static const QRegularExpression line(QStringLiteral("^\\s*(UID_MIN|UID_MAX)\\s+(\\S+)"), QRegularExpression::MultilineOption);

    auto it = line.globalMatch(content);
    while (it.hasNext()) {
        const auto match = it.next();
        bool ok = false;
        const unsigned value = match.captured(2).toUInt(&ok);
        if (!ok) {
            continue;
        }
        if (match.captured(1) == QLatin1String("UID_MIN")) {
            range.min = value;
        } else {
            range.max = value;
        }
    }
    return range;
}

inline bool isRealUser(unsigned uid, QString shell, const UidRange &range, const QStringList &loginShells)
{
    if (uid < range.min || uid > range.max) {
        return false;
    }

    // An empty shell field means /bin/sh (passwd(5)).
    if (shell.isEmpty()) {
        shell = QStringLiteral("/bin/sh");
    }

    // Some systems list these in /etc/shells, they still mean "no login".
    if (shell.endsWith(QLatin1String("/nologin")) || shell.endsWith(QLatin1String("/false"))) {
        return false;
    }

    return loginShells.contains(shell);
}

}
