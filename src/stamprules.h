/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Reads the rubber stamp rules of Howdy's config.ini. mINI, which loads the
// rest of the file, does not know values spread over several lines, and
// stamp_rules is one: this reads it the way Python's configparser does for Howdy.
namespace StampRules
{

// The rules of stamp_rules in [rubberstamps], one per line, without the blank
// and comment lines.
inline QStringList parse(const QString &content)
{
    static const QRegularExpression section(QStringLiteral("^\\[(.+)\\]"));
    static const QRegularExpression entry(QStringLiteral("^([^=:]+?)\\s*[=:]\\s*(.*)$"));

    QStringList rules;
    const auto addRule = [&rules](const QString &text) {
        const QString rule = text.trimmed();
        if (!rule.isEmpty()) {
            rules.append(rule);
        }
    };

    bool inSection = false;
    bool inValue = false;
    for (const QString &line : content.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        // configparser skips these without ending a value
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')) || trimmed.startsWith(QLatin1Char(';'))) {
            continue;
        }
        // indented lines continue the value above
        if (line.front().isSpace()) {
            if (inValue) {
                addRule(trimmed);
            }
            continue;
        }

        inValue = false;
        const auto header = section.match(trimmed);
        if (header.hasMatch()) {
            inSection = header.captured(1) == QLatin1String("rubberstamps");
            continue;
        }
        const auto match = entry.match(trimmed);
        if (inSection && match.hasMatch() && match.captured(1).toLower() == QLatin1String("stamp_rules")) {
            addRule(match.captured(2));
            inValue = true;
        }
    }
    return rules;
}

// The timeout in seconds when the rules are a single nod that denies the
// unlock once it times out, the only stamp setup this module offers; -1 for
// anything else, including options the nod stamp would refuse.
inline double nodTimeout(const QStringList &rules)
{
    static const QRegularExpression nod(
        QStringLiteral("^nod\\s+(\\d+(?:\\.\\d+)?)s?\\s+failsafe(?:\\s+(?:min_distance|min_directions)=\\d+)*$"));

    if (rules.size() != 1) {
        return -1;
    }
    const auto match = nod.match(rules.first());
    if (!match.hasMatch()) {
        return -1;
    }
    const double timeout = match.captured(1).toDouble();
    return timeout > 0 ? timeout : -1;
}

}
