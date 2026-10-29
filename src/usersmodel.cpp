/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "usersmodel.h"
#include "config.h"
#include "mini/ini.h"
#include "usersfilter.h"

#include <KLocalizedString>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

#include <algorithm>

#include <pwd.h>
#include <unistd.h>

// Same pictures as the Users page of System Settings
static const QLatin1String AVATARS_PATH("/var/lib/AccountsService/icons");

UsersModel::UsersModel(QObject *parent) : QAbstractListModel(parent) {}

int UsersModel::countModels(const QString &userName)
{
    QFile file(QStringLiteral("%1/%2.dat").arg(QStringLiteral(HOWDY_MODELS_PATH), userName));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isArray() ? document.array().size() : 0;
}

void UsersModel::refreshModelCounts()
{
    for (auto &user : m_users) {
        user.modelCount = countModels(user.name);
    }
    if (!m_users.isEmpty()) {
        Q_EMIT dataChanged(index(0), index(m_users.size() - 1), { ModelCountRole });
    }
}

QString UsersModel::fillUsers()
{
    mINI::INIFile file(MODULE_CONFIG_FILE);
    mINI::INIStructure ini;
    unsigned minimumUid = file.read(ini) ? atoi(ini["UserList"]["minimum-uid"].c_str()) : 500;

    m_currentUserIndex = -1;

    UsersFilter::UidRange range;
    QFile loginDefs(QStringLiteral("/etc/login.defs"));
    if (loginDefs.open(QIODevice::ReadOnly | QIODevice::Text)) {
        range = UsersFilter::parseLoginDefs(QString::fromLocal8Bit(loginDefs.readAll()));
    }
    // the module's own setting can only narrow the range
    range.min = std::max(range.min, minimumUid);

    QStringList loginShells;
    setusershell();
    while (const char *shell = getusershell()) {
        loginShells << QString::fromLocal8Bit(shell);
    }
    endusershell();

    auto pw = getpwuid(getuid());
    QString currentUserName;
    if (!pw || strlen(pw->pw_name) == 0) {
        return i18n("Unable to get user data");
    } else {
        currentUserName = QLatin1String(pw->pw_name);
    }

    setpwent();
    while (auto entry = getpwent()) {

        // Ignore system and service accounts
        if (!UsersFilter::isRealUser(entry->pw_uid, QString::fromLocal8Bit(entry->pw_shell), range, loginShells)) continue;

        UserEntry newUser;
        newUser.name = QString::fromLatin1(entry->pw_name);
        // GECOS is "Full Name,room,phone,...": keep the name only
        newUser.realName = QString::fromUtf8(entry->pw_gecos).section(QLatin1Char(','), 0, 0).trimmed();

        if (newUser.realName.isEmpty()) newUser.realName = newUser.name;

        const QFileInfo avatar(QStringLiteral("%1/%2").arg(AVATARS_PATH, newUser.name));
        if (avatar.isFile() && avatar.isReadable()) {
            newUser.avatar = QUrl::fromLocalFile(avatar.filePath()).toString();
        }
        newUser.modelCount = countModels(newUser.name);
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

        case AvatarRole:
            return m_users[row].avatar;

        case ModelCountRole:
            return m_users[row].modelCount;
    }
    return {};
}

QVariantMap UsersModel::get(int row) const
{
    if (row < 0 || row >= m_users.size()) {
        return {};
    }
    const auto &user = m_users[row];
    return {
        { QStringLiteral("name"), user.name },
        { QStringLiteral("realName"), user.realName },
        { QStringLiteral("avatar"), user.avatar },
        { QStringLiteral("modelCount"), user.modelCount },
    };
}

QHash<int, QByteArray> UsersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[RealNameRole] = "realName";
    roles[AvatarRole] = "avatar";
    roles[ModelCountRole] = "modelCount";
    return roles;
}
