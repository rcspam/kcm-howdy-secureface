/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "kcm_config.h"

#include <QVariant>
#include <QQuickItem>

#include <KQuickAddons/ManagedConfigModule>
#include <KAuth/Action>
#include <KAuth/ExecuteJob>

#define TRANSLATION_DOMAIN "kcm_howdy"
#include <KLocalizedString>

#include "mini/ini.h"

KcmConfig::KcmConfig(QString filename, KQuickAddons::ManagedConfigModule *parent) : QObject(parent), m_filename(filename) {}

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

void KcmConfig::load()
{
    mINI::INIFile file(qPrintable(m_filename));
    mINI::INIStructure ini;
    m_storedConfig.clear();
    m_updatedConfig.clear();
    auto p = static_cast<KQuickAddons::ManagedConfigModule*>(parent());
    if(!file.read(ini)) {
        qWarning("%s: can't read config file %s", Q_FUNC_INFO, qPrintable(m_filename));
        Q_EMIT errorLoadConfigFile(m_filename);
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

static QString deniedIfEmpty(QString errorString)
{
    return errorString.length() == 0 ? i18n("Access denied") : errorString;
}

void KcmConfig::save()
{
    QVariantMap args;
    for (auto i = m_storedConfig.begin(); i != m_storedConfig.end(); ++i) {
        args[i.key()] = i.value();
    }
    // overwrite the changed values, if any
    for (auto i = m_updatedConfig.begin(); i != m_updatedConfig.end(); ++i) {
        args[i.key()] = i.value();
    }

    auto p = static_cast<KQuickAddons::ManagedConfigModule*>(parent());

    KAuth::Action saveAction(QStringLiteral("org.kde.kcontrol.kcmhowdy.saveconfig"));
    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmhowdy"));
    saveAction.setArguments(args);
    KAuth::ExecuteJob *job = saveAction.execute();
    if (!job->exec())
    {
        qWarning() << "Save failed:" << job->errorText() << ", " << job->errorString();
        Q_EMIT errorSaveConfigFile(deniedIfEmpty(job->errorString()));
        p->setNeedsSave(true);
    }
    else
    {
        for (auto i = m_updatedConfig.begin(); i != m_updatedConfig.end(); ++i) {
            m_storedConfig[i.key()] = i.value();
        }
    }
}
