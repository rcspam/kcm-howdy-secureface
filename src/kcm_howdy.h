/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

class KcmConfig;

class KcmHowdy: public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
public:
    explicit KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    Q_PROPERTY(KcmConfig *config MEMBER m_config CONSTANT)

public Q_SLOTS:
    void load() override;
    void save() override {};
    void defaults() override {};

private:
    KcmConfig *m_config;
};
