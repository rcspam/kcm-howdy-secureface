/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "usersmodel.h"

#include <KLocalizedString>

#include <pwd.h>
#include <unistd.h>

UsersModel::UsersModel(QObject *parent) : QAbstractListModel(parent) {}

QString UsersModel::fillUsers()
{
    const int minimumUid = 500;
    m_currentUserIndex = -1;

    QStringList hiddenShells{ QStringLiteral("/bin/false"), QStringLiteral("/sbin/nologin"), QStringLiteral("/bin/nologin"), QStringLiteral("/dev/null") };

    auto pw = getpwuid(getuid());
    QString currentUserName;
    if (!pw || strlen(pw->pw_name) == 0) {
        return i18n("Unable to get user data");
    } else {
        currentUserName = QLatin1String(pw->pw_name);
    }

    setpwent();
    while (auto entry = getpwent()) {

        // Ignore system users
        if (entry->pw_uid < minimumUid) continue;

        // Ignore users disabled by shell
        if (hiddenShells.contains(QString::fromLatin1(entry->pw_shell))) continue;

        UserEntry newUser;
        newUser.name = QString::fromLatin1(entry->pw_name);
        newUser.realName = QString::fromLatin1(entry->pw_gecos);

        if (newUser.realName.isEmpty()) newUser.realName = newUser.name;
        if (currentUserName == newUser.name) m_currentUserIndex = m_users.size();

        m_users << newUser;
    }
    endpwent();

    if (m_currentUserIndex < 0) return i18n("User unavailable");
    return {};
}

int UsersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_users.size();
}

QVariant UsersModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    switch (role) {

        case NameRole:
            return m_users[row].name;

        case RealNameRole:
            return m_users[row].realName;
    }
    return {};
}

QHash<int, QByteArray> UsersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[RealNameRole] = "realName";
    return roles;
}
