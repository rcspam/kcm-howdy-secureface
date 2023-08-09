/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QMap>
#include <QVariant>

namespace KQuickAddons {
    class ManagedConfigModule;
}

class KcmConfig: public QObject
{
    Q_OBJECT

public:
    explicit KcmConfig(KQuickAddons::ManagedConfigModule *parent);

    Q_INVOKABLE QVariant getConfigValue(QString url);
    Q_INVOKABLE void updateConfigValue(QString url, QVariant value);
    Q_INVOKABLE void storeDefaultValue(QString url, QVariant value);
    void load(const char *filename);

Q_SIGNALS:
    void configLoaded();
    void errorLoadConfigFile(QString filename);

private:
    using Config = QMap<QString, QString>;

    bool hasSomethingNew(Config &newCfg, Config &oldCfg);

    Config m_storedConfig;
    Config m_updatedConfig;
};
