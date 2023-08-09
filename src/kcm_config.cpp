/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "kcm_config.h"

#include <QVariant>
#include <QQuickItem>

#include <KQuickAddons/ManagedConfigModule>

#include "mini/ini.h"

KcmConfig::KcmConfig(KQuickAddons::ManagedConfigModule *parent) : QObject(parent) {}

bool KcmConfig::hasSomethingNew(Config &newCfg, Config &oldCfg)
{
    for (auto i = newCfg.begin(); i != newCfg.end(); ++i) {
        if (!oldCfg.contains(i.key()) || oldCfg[i.key()] != i.value()) {
            return true;
        }
    }
    return false;
}

void KcmConfig::updateConfigValue(QString url, QVariant value)
{
    m_updatedConfig[url] = value.toString();
    bool needSave = hasSomethingNew(m_updatedConfig, m_storedConfig);
    static_cast<KQuickAddons::ManagedConfigModule*>(parent())->setNeedsSave(needSave);
}

void KcmConfig::storeDefaultValue(QString url, QVariant value)
{
    if (!m_storedConfig.contains(url)) {
        m_storedConfig[url] = value.toString();
    }
}

QVariant KcmConfig::getConfigValue(QString url)
{
    if (m_storedConfig.contains(url)) {
        return m_storedConfig[url];
    } else {
        return {};
    }
}

void KcmConfig::load(const char *filename)
{
    mINI::INIFile file(filename);
    mINI::INIStructure ini;
    m_storedConfig.clear();
    m_updatedConfig.clear();
    auto p = static_cast<KQuickAddons::ManagedConfigModule*>(parent());
    if(!file.read(ini)) {
        qWarning("%s: can't read config file %s", Q_FUNC_INFO, filename);
        Q_EMIT errorLoadConfigFile(QLatin1String(filename));
        return;
    }
    for (auto &group : ini) {
        for (auto &entry : group.second) {
            QString entryPath = QStringLiteral("%1/%2").arg(QString::fromStdString(group.first)).arg(QString::fromStdString(entry.first));
            m_storedConfig[entryPath] = QString::fromStdString(entry.second);
        }
    }
    Q_EMIT configLoaded();
    p->setNeedsSave(false);
}
