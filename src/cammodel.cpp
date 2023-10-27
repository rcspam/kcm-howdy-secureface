/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "cammodel.h"

#include <QCameraInfo>

#include <KLocalizedString>

CamModel::CamModel(QObject *parent) : QAbstractListModel(parent)
{
    m_cams = {{{}, i18n("No camera selected") }};

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        m_cams.push_back({
            cameraInfo.deviceName(),
            QStringLiteral("%1 (%2)").arg(cameraInfo.description()).arg(cameraInfo.deviceName())
        });
    }
}

int CamModel::rowCount(const QModelIndex&) const
{
    return m_cams.size();
}

QVariant CamModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    switch (role) {

        case NameRole:
            return m_cams[row].name;

        case DescriptionRole:
            return m_cams[row].description;
    }
    return QVariant();
}

QHash<int, QByteArray> CamModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    return roles;
}
