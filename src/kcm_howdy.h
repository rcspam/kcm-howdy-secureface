/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

class KcmConfig;
class FacesModel;
class CamModel;

class KcmHowdy: public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
public:
    explicit KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    Q_PROPERTY(KcmConfig *config MEMBER m_config CONSTANT)
    Q_PROPERTY(int gridUnit READ gridUnit CONSTANT)
    Q_PROPERTY(FacesModel *facesModel MEMBER m_facesModel CONSTANT)
    Q_PROPERTY(CamModel *camModel MEMBER m_camModel CONSTANT)

Q_SIGNALS:
    void errorInitBackend(QString message);

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override {};

private:
    KcmConfig *m_config;
    FacesModel *m_facesModel;
    CamModel *m_camModel;
    int m_gridUnit = 0;
    int gridUnit();
};
