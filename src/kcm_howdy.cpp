/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/


#include "kcm_howdy.h"
#include "config.h"

#include <QQuickItem>
#include <QGuiApplication>
#include <QFontMetrics>
#include <QCameraInfo>

#include <KPluginFactory>
#include <KLocalizedString>

#include "kcm_config.h"
#include "facesmodel.h"
#include "cammodel.h"
#include "usersmodel.h"
#include "previewproducer.h"
#include "qpixmapitem.h"

K_PLUGIN_CLASS_WITH_JSON(KcmHowdy, "kcm_howdy.json")

KcmHowdy::KcmHowdy(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_config(new KcmConfig(QStringLiteral(HOWDY_CONFIG_FILE), this))
{
    qmlRegisterAnonymousType<KcmConfig>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<FacesModel>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<CamModel>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<CamModelEmptyItemProxy>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<UsersModel>("org.altlinux.howdy.kcm", 1);
    qmlRegisterAnonymousType<PreviewProducer>("org.altlinux.howdy.kcm", 1);

    qmlRegisterType<QPixmapItem>("org.altlinux.howdy.kcm.custom", 1, 0, "QPixmapItem");
    qmlRegisterUncreatableType<ConfigModule>("org.altlinux.howdy.kcm.configmodule", 1, 0, "ConfigModule", QStringLiteral("Only for enums"));

    QString errorString;
    // We remember only the first error that appears
    auto consumeError = [&errorString] (QString &&error) {
        if (errorString.isEmpty()) errorString = error;
    };

    m_usersModel = new UsersModel(this);
    consumeError(m_usersModel->fillUsers());

    m_facesModel = new FacesModel(this);

    m_camModel = new CamModel(this);

    if (m_camModel->rowCount(QModelIndex()) == 0) {
        consumeError(i18n("No cameras found"));
    }

    m_previewProducer = new PreviewProducer(m_config, this);

    setButtons(buttons() & ~Button::Default);

    // it may fail to initialize, if, for example, there is an error in the QML
    auto qmlRoot = mainUi();
    if (!qmlRoot) return;
    qmlRoot->setProperty("errorString", errorString);
    QDir modelsDir(QStringLiteral(HOWDY_MODELS_PATH));
    qmlRoot->setProperty("enableOnboarding", !modelsDir.exists());
}

void KcmHowdy::load()
{
    m_config->load();
}

void KcmHowdy::save()
{
    m_config->save();
}

void KcmHowdy::defaults()
{
    m_config->defaults();
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
