/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QAbstractProxyModel>

class CamModelEmptyItemProxy;
class QMediaDevices;

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

    struct CameraEntry {
        QString name;
        QString description;
        bool infrared = false;
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    // to extract by role an entry that is not in the model
    QVariant dataFromEntry(const CameraEntry &entry, int role) const;
    QHash<int, QByteArray> roleNames() const override;

    CamModelEmptyItemProxy *getEmptyItemProxy();

    Q_PROPERTY(CamModelEmptyItemProxy *emptyItemProxy READ getEmptyItemProxy CONSTANT)
    Q_INVOKABLE QVariant getItemProperty(int ind, QByteArray name)
    {
        QModelIndex qi = index(ind, 0);
        return data(qi, m_invRoleNames[name]);
    }
    // the name of the camera the path leads to, or the path itself if none:
    // a config holding /dev/videoN or another link still selects its camera
    Q_INVOKABLE QString matchingName(const QString &path) const;

    // bumped on every new list, for bindings that look cameras up by name
    Q_PROPERTY(int revision MEMBER m_revision NOTIFY revisionChanged)
    // looks for cameras again; also runs when one is plugged in or out
    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void revisionChanged();

private:

    QMediaDevices *m_mediaDevices;
    int m_revision = 0;
    QList<CameraEntry> m_cams;
    QHash<QByteArray, int> m_invRoleNames;
    CamModelEmptyItemProxy *m_emptyItemProxy = nullptr;
};

class CamModelEmptyItemProxy : public QAbstractProxyModel
{
    Q_OBJECT

public:
    explicit CamModelEmptyItemProxy(CamModel *source);

    Q_INVOKABLE QVariant getItemProperty(int ind, QByteArray name)
    {
        if (ind == 0) return {};
        return static_cast<CamModel*>(sourceModel())->getItemProperty(ind - 1, name);
    }

    // QAbstractProxyModel
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    // QAbstractItemModel
    QModelIndex index(int row, int col, const QModelIndex&) const override { return createIndex(row, col); }
    QModelIndex parent(const QModelIndex&) const override { return QModelIndex(); }
    int rowCount(const QModelIndex&) const override { return sourceModel()->rowCount() + 1; }
    int columnCount(const QModelIndex&) const override { return 1; }
private:
    static CamModel::CameraEntry s_noCameraItem;
};
