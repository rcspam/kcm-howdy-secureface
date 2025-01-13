/*
    SPDX-FileCopyrightText: 2023-2025 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <KQuickManagedConfigModule>

#include "previewproducer.h"
#include "usersmodel.h"

class KcmConfig;
class FacesModel;
class CamModel;

class KcmHowdy: public KQuickManagedConfigModule
{
    Q_OBJECT
public:
    explicit KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    Q_PROPERTY(KcmConfig *config MEMBER m_config CONSTANT)
    Q_PROPERTY(int gridUnit READ gridUnit CONSTANT)
    Q_PROPERTY(FacesModel *facesModel MEMBER m_facesModel CONSTANT)
    Q_PROPERTY(CamModel *camModel MEMBER m_camModel CONSTANT)
    Q_PROPERTY(UsersModel *usersModel MEMBER m_usersModel CONSTANT)
    Q_PROPERTY(PreviewProducer *previewProducer MEMBER m_previewProducer NOTIFY previewProducerUpdated)
    Q_PROPERTY(bool enableOnboarding MEMBER m_enableOnboarding CONSTANT)
    Q_PROPERTY(QString errorString MEMBER m_errorString CONSTANT)

Q_SIGNALS:
    void errorInitBackend(QString message);
    void previewProducerUpdated();

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private:
    KcmConfig *m_config;
    FacesModel *m_facesModel;
    CamModel *m_camModel;
    UsersModel *m_usersModel;
    PreviewProducer *m_previewProducer;
    int m_gridUnit = 0;
    int gridUnit();
    bool m_enableOnboarding = false;
    QString m_errorString;
};
