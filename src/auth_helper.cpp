/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "auth_helper.h"
#include "config.h"

#include <QProcess>
#include <KAuth/HelperSupport>
#include <KLocalizedString>
#include <pwd.h>

#include "mini/ini.h"

KAuth::ActionReply AuthHelper::addfacemodel(const QVariantMap &args)
{
    QString modelName = args[QStringLiteral("modelName")].toString();
    QStringList params{ QStringLiteral("add") };

    if (modelName.length() == 0) {
        params.append(QStringLiteral("-y"));
    }

    return invokeForCallingUser(params, modelName);
}

KAuth::ActionReply AuthHelper::removefacemodel(const QVariantMap &args)
{
    QString modelID = args[QStringLiteral("modelID")].toString();
    QStringList params{ QStringLiteral("remove"), modelID, QStringLiteral("-y") };
    return invokeForCallingUser(params);
}

KAuth::ActionReply AuthHelper::saveconfig(const QVariantMap &args)
{
    if (KAuth::HelperSupport::isStopped()) {
        return KAuth::ActionReply::HelperErrorReply();
    }
    KAuth::ActionReply errorReply = KAuth::ActionReply::HelperErrorReply();

    mINI::INIFile file(HOWDY_CONFIG_FILE);
    mINI::INIStructure ini;

    for (auto i = args.constBegin() ; i != args.constEnd() ; ++i)
    {
        QStringList lst = i.key().split(QLatin1Char('/'));
        if (lst.size() != 2)
        {
            errorReply.setErrorDescription(i18n("Invalid key format: %1", i.key()));
            return errorReply;
        }

        std::string groupName = lst[0].toStdString();
        std::string keyName = lst[1].toStdString();
        std::string value = i.value().toString().toStdString();

        ini[groupName][keyName] = value;

    }

    if (!file.write(ini)) {
        errorReply.setErrorDescription(i18n("Failed to write configuration file on disk"));
        return errorReply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply AuthHelper::error(QString errorText) {
    auto errorReplyAction = KAuth::ActionReply::HelperErrorReply();
    errorReplyAction.setErrorDescription(errorText);
    return errorReplyAction;
}

KAuth::ActionReply AuthHelper::invokeForCallingUser(QStringList params, QString message)
{
    if (KAuth::HelperSupport::isStopped()) {
        return KAuth::ActionReply::HelperErrorReply();
    }
    int uid = KAuth::HelperSupport::callerUid();

    auto pw = getpwuid(uid);
    QString userName;
    if (!pw || strlen(pw->pw_name) == 0) {
        return error(i18n("Unable to get user data"));
    } else {
        userName = QLatin1String(pw->pw_name);
    }

    params.append(QStringLiteral("--user"));
    params.append(userName);

    QString command = QStringLiteral("howdy");
    QProcess howdy;
    howdy.start(command, params);
    howdy.waitForStarted();
    howdy.waitForReadyRead();

    if (howdy.exitCode()) {
        return error(i18n("Howdy is not installed correctly"));
    }

    if (message.length() != 0) {
        howdy.write(message.toLatin1());
        howdy.waitForBytesWritten();
        howdy.closeWriteChannel();
    }

    howdy.waitForFinished();

    QString replyFromCommand(QString::fromLocal8Bit(howdy.readAllStandardOutput()));
    QStringList linesofReply = replyFromCommand.split(QStringLiteral("\n"));
    QString replyMessage = linesofReply.at(linesofReply.size() - 2);

    if (howdy.exitCode()) {
        return error(replyMessage);
    }
    return KAuth::ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmhowdy", AuthHelper)
