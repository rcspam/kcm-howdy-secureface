/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "kcm_howdy.h"

#include <QQuickItem>
#include <QGuiApplication>
#include <QFontMetrics>

#include <KPluginFactory>
#define TRANSLATION_DOMAIN "kcm_howdy"
#include <KLocalizedString>

#include <unistd.h>
#include <pwd.h>

#include "kcm_config.h"
#include "facesmodel.h"

K_PLUGIN_CLASS_WITH_JSON(KcmHowdy, "kcm_howdy.json")

KcmHowdy::KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_config(new KcmConfig(this))
{
    qmlRegisterAnonymousType<KcmConfig>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<FacesModel>("org.altlinux.howdy.kcm", 1);

    QString username;
    auto pw = getpwuid(getuid());
    if (!pw || strlen(pw->pw_name) == 0) {
        Q_EMIT errorInitBackend(i18n("Unable to get user data"));
        return;
    } else {
        username = QLatin1String(pw->pw_name);
    }

    m_facesModel = new FacesModel(QStringLiteral("%1/%2.dat")
                                  .arg(QStringLiteral(HOWDY_MODELS_PATH))
                                  .arg(username), this);
}

void KcmHowdy::load()
{
    m_config->load(HOWDY_CONFIG_FILE);
}


int KcmHowdy::gridUnit()
{
    if (m_gridUnit > 0) return m_gridUnit;

    // taken from KDE plasma-framework,
    // src/declarativeimports/core/units.cpp

    m_gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (m_gridUnit % 2 != 0) {
        m_gridUnit++;
    }

    return m_gridUnit;
}

#include "kcm_howdy.moc"
