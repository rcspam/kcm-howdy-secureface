/*
    SPDX-FileCopyrightText: 2023-2025 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QMap>
#include <QVariant>

class KQuickManagedConfigModule;

class KcmConfig: public QObject
{
    Q_OBJECT

public:
    explicit KcmConfig(QString fileName, KQuickManagedConfigModule *parent);

    Q_INVOKABLE QVariant getConfigValue(QString url);
    Q_INVOKABLE void updateConfigValue(QString url, QVariant value);
    Q_INVOKABLE void storeDefaultValue(QString url, QVariant value);
    // seconds of the nod stamp rule, or -1 when the rules are anything else
    Q_INVOKABLE double nodTimeout() const;
    // return false if error occurs
    Q_INVOKABLE bool save();
    void load();
    void defaults();

Q_SIGNALS:
    void configLoaded();
    void configSaved();
    void setDefaults();
    void errorLoadConfigFile(QString filename);
    void errorSaveConfigFile(QString message);

private:
    using Config = QMap<QString, QString>;

    bool hasSomethingNew(Config &newCfg, Config &oldCfg);
    KQuickManagedConfigModule *client();

    QString m_filename;
    Config m_storedConfig;
    Config m_updatedConfig;
    double m_nodTimeout = -1;
};
