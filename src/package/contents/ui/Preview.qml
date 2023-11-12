/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.altlinux.howdy.kcm.custom 1.0 as Custom

Rectangle {
    id: preview

    property var enabled: false

    width: videoOut.width + gap * 2
    height: videoOut.height + gap * 2

    radius: gap
    color: paletteActive.base
    border.color: paletteActive.mid
    border.width: 1

    Custom.QPixmapItem {
        id: videoOut
        x: gap
        y: gap
        width: kcm.gridUnit * 20
        height: kcm.gridUnit * 15
        pixmap: kcm.previewProducer.pixmap
        smooth: true
        fillMode: Custom.QPixmapItem.PreserveAspectFit
        visible: preview.enabled && !kcm.previewProducer.waitingCamera
    }

    Rectangle {
        id: blackScreen
        visible: !preview.enabled
        color: "black"
        anchors.fill: videoOut
    }

    BusyIndicator {
        anchors.centerIn: preview
        running: true
        visible: preview.enabled && kcm.previewProducer.waitingCamera
    }

    onEnabledChanged: kcm.previewProducer.capturing = enabled
}
