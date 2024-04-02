/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.altlinux.howdy.kcm.custom 1.0 as Custom

Rectangle {
    id: preview

    property var enabled: false
    property string message: ""

    property int defaultWidth: kcm.gridUnit * 20 + gap * 2
    property int defaultHeight: kcm.gridUnit * 15 + gap * 2

    width: defaultWidth
    height: defaultHeight

    radius: gap
    color: paletteActive.base
    border.color: paletteActive.mid
    border.width: 1

    Item {
        anchors.fill: parent
        visible: message.length == 0

        Custom.QPixmapItem {
            id: videoOut
            anchors.fill: parent
            anchors.margins: gap
            pixmap: kcm.previewProducer.pixmap
            smooth: true
            fillMode: Custom.QPixmapItem.PreserveAspectFit
            visible: preview.enabled && !kcm.previewProducer.waitingCamera
        }

        Rectangle {
            id: blackScreen
            anchors.fill: videoOut
            visible: !preview.enabled
            color: "black"
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: true
            visible: preview.enabled && kcm.previewProducer.waitingCamera
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: gap
        visible: message.length != 0

        Text {
            anchors.centerIn: parent
            text: preview.message
        }
    }
}
