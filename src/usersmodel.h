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
        AvatarRole,
        ModelCountRole,
    };

    explicit UsersModel(QObject *parent = nullptr);
    QString fillUsers();

    Q_PROPERTY(int currentUserIndex MEMBER m_currentUserIndex CONSTANT)

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // the roles of one row, to open a user page without going through a delegate
    Q_INVOKABLE QVariantMap get(int row) const;

public Q_SLOTS:
    // re-read the number of face models of every user, after an add or a remove
    void refreshModelCounts();

private:

    static int countModels(const QString &userName);

    struct UserEntry {
        QString name;
        QString realName;
        QString avatar;
        int modelCount = 0;
    };

    QList<UserEntry> m_users;
    int m_currentUserIndex;
};
