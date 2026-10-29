/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

#include <algorithm>

// Camera paths as Howdy's config.ini holds them: a /dev/videoN node, whose
// number can change between boots, or a link to it such as the ones in
// /dev/v4l/by-path that Howdy advises.
namespace DeviceLinks
{

inline const QString BY_PATH = QStringLiteral("/dev/v4l/by-path");

// True when both paths lead to the same existing device node
inline bool sameDevice(const QString &a, const QString &b)
{
    const QString nodeA = QFileInfo(a).canonicalFilePath();
    return !nodeA.isEmpty() && nodeA == QFileInfo(b).canonicalFilePath();
}

// The link of linkDir that leads to device, the first in name order so the
// choice does not depend on the directory listing; empty when there is none
inline QString stableLink(const QString &device, const QString &linkDir = BY_PATH)
{
    QFileInfoList links = QDir(linkDir).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot);
    std::sort(links.begin(), links.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.fileName() < b.fileName();
    });
    for (const QFileInfo &link : std::as_const(links)) {
        if (sameDevice(link.absoluteFilePath(), device)) {
            return link.absoluteFilePath();
        }
    }
    return {};
}

}
