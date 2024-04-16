/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QAbstractListModel>

class UsersModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        NameRole = Qt::UserRole,
        RealNameRole,
    };

    explicit UsersModel(QObject *parent = nullptr);
    QString fillUsers();

    Q_PROPERTY(int currentUserIndex MEMBER m_currentUserIndex CONSTANT)

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:

    struct UserEntry {
        QString name;
        QString realName;
    };

    QList<UserEntry> m_users;
    int m_currentUserIndex;
};
