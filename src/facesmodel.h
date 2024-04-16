/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QAbstractTableModel>
#include <QFile>

namespace KAuth {
    class Action;
}

class FacesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit FacesModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Q_PROPERTY(int rows READ rowCount NOTIFY modelUpdated)

    Q_INVOKABLE void addFace(QString modelName);
    Q_INVOKABLE void addOtherFace(QString userName, QString modelName);
    Q_INVOKABLE void removeFace(int row);
    Q_INVOKABLE void removeOtherFace(QString userName, int row);
    Q_INVOKABLE QVariant getCell(int row, int col)
    {
        return data(index(row, col), Qt::DisplayRole);
    }
    Q_INVOKABLE void setUserName(QString userName);

Q_SIGNALS:

    void errorCreateFacesModel(QString message);
    void errorAction(QString message);
    void successAddFace();
    void authorizedForAddFace();
    void modelUpdated();

private:

    void Update();

    void actionAddFace(KAuth::Action &action);
    void actionRemoveFace(KAuth::Action &action);

    struct FaceEntry {
        uint time;
        QString label;
        uint id;
    };

    QList<FaceEntry> m_faces;
    QFile m_file;
};
