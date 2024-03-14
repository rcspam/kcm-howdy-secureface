/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QAbstractListModel>

class CamModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        NameRole = Qt::UserRole,
        DescriptionRole,
        InfraredRole
    };

    explicit CamModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariant getItemProperty(int ind, QByteArray name)
    {
        QModelIndex qi = index(ind, 0);
        return data(qi, m_invRoleNames[name]);
    }

private:

    struct CameraEntry {
        QString name;
        QString description;
        bool infrared = false;
    };

    QList<CameraEntry> m_cams;
    QHash<QByteArray, int> m_invRoleNames;
};
