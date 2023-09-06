/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <KAuth/ActionReply>

class AuthHelper: public QObject
{
    Q_OBJECT

public Q_SLOTS:
    KAuth::ActionReply addfacemodel(const QVariantMap &args);
    KAuth::ActionReply removefacemodel(const QVariantMap &args);
    KAuth::ActionReply saveconfig(const QVariantMap &args);

private:
    KAuth::ActionReply error(QString errorText);
    KAuth::ActionReply invokeForCallingUser(QStringList params, QString pipeMessage = {});
};
