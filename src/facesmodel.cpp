/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "facesmodel.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <KAuth/Action>
#include <KAuth/ExecuteJob>
#include <KLocalizedString>

FacesModel::FacesModel(QString fileName, QObject *parent) : QAbstractTableModel(parent)
{
    m_file.setFileName(fileName);
    Update();
}

void FacesModel::Update()
{
    struct resetModelGuard {

        FacesModel *facesModel;

        resetModelGuard(FacesModel *facesModel) : facesModel(facesModel)
        {
            facesModel->beginResetModel();
            facesModel->m_faces.clear();
        }
        ~resetModelGuard()
        {
            facesModel->endResetModel();
            Q_EMIT facesModel->modelUpdated();
        }
    };

    resetModelGuard guard(this);

    if (!m_file.exists()) {
        return;
    }

    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Q_EMIT errorCreateFacesModel(i18n("Model file exists but cannot be opened"));
        return;
    }

    QString fileContent = QString::fromLocal8Bit(m_file.readAll());
    m_file.close();

    QJsonDocument document = QJsonDocument::fromJson(fileContent.toUtf8());
    if (!document.isArray()) {
        Q_EMIT errorCreateFacesModel(i18n("Invalid model file"));
        return;
    }

    const QJsonArray models = document.array();

    for (auto model : models) {

        FaceEntry face;

        face.time = model[QStringLiteral("time")].toDouble();
        face.label = model[QStringLiteral("label")].toString();
        face.id = model[QStringLiteral("id")].toDouble();

        m_faces.append(face);
    }
}

int FacesModel::rowCount(const QModelIndex&) const
{
    return m_faces.size();
}

int FacesModel::columnCount(const QModelIndex&) const
{
    return 3;
}

QVariant FacesModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    switch (role) {

        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;

        case Qt::DisplayRole:
            switch (col) {
                case 0: return m_faces.at(row).id;
                case 1:
                {
                    QDateTime timestamp;
                    timestamp.setTime_t(m_faces.at(row).time);
                    return timestamp.toString(QLocale().dateTimeFormat(QLocale::ShortFormat));;
                }
                case 2: return m_faces.at(row).label;
            }
    }
    return {};
}

QVariant FacesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal){
        switch (section) {
            case 0: return i18n("Id");
            case 1: return i18n("Time");
            case 2: return i18n("Name");
        }
    }
    return {};
}

static QString deniedIfEmpty(QString errorString)
{
    return errorString.length() == 0 ? i18n("Access denied") : errorString;
}

void FacesModel::addFace(QString modelName)
{
    QVariantMap args;
    args[QStringLiteral("modelName")] = modelName;
    KAuth::Action addAction(QStringLiteral("org.kde.kcontrol.kcmhowdy.addfacemodel"));

    addAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmhowdy"));
    addAction.setArguments(args);

    auto job = addAction.execute();

    if (addAction.status() == KAuth::Action::AuthorizedStatus) {
        Q_EMIT authorizedForAddFace();
    } else {
        connect(job, &KAuth::ExecuteJob::statusChanged, [this](KAuth::Action::AuthStatus status) {
            if (status == KAuth::Action::AuthorizedStatus) {
                Q_EMIT authorizedForAddFace();
            }
        });
    }

    if (!job->exec())
    {
        qWarning() << "Add face model failed:" << job->errorText() << ", " << job->errorString();
        job->kill();
        Q_EMIT errorAction(QStringLiteral("%1\n%2").arg(i18n("Add face model failed:")).arg(deniedIfEmpty(job->errorString())));
    }
    else
    {
        Update();
        Q_EMIT successAddFace();
    }
}

void FacesModel::removeFace(int row)
{
    QVariantMap args;

    int id = m_faces.at(row).id;
    args[QStringLiteral("modelID")] = id;
    KAuth::Action removeAction(QStringLiteral("org.kde.kcontrol.kcmhowdy.removefacemodel"));

    removeAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmhowdy"));
    removeAction.setArguments(args);

    auto job = removeAction.execute();

    if (!job->exec())
    {
        qWarning() << "Remove face model failed:" << job->errorText() << ", " << job->errorString();
        job->kill();
        Q_EMIT errorAction(QStringLiteral("%1\n%2").arg(i18n("Remove face model failed:")).arg(deniedIfEmpty(job->errorString())));
    }
    else
    {
        Update();
    }
}
