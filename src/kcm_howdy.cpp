/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "kcm_howdy.h"

#include <QQuickItem>

#include <KPluginFactory>
#include "kcm_config.h"

K_PLUGIN_CLASS_WITH_JSON(KcmHowdy, "kcm_howdy.json")

KcmHowdy::KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_config(new KcmConfig(this))
{
    qmlRegisterAnonymousType<KcmConfig>("org.altlinux.howdy.kcm", 1);
}

void KcmHowdy::load()
{
    m_config->load("/etc/howdy/config.ini");
}

#include "kcm_howdy.moc"
