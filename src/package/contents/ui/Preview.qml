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

    property int prevPaintedHeight: 0
    property int prevPaintedWidth: 0

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

            onPaintedWidthChanged: if (visible && prevPaintedWidth != paintedWidth) {
                updateFontParams()
            }

            onPaintedHeightChanged: if (visible && prevPaintedHeight != paintedHeight) {
                updateFontParams()
            }

            onVisibleChanged: if (visible) {
                updateFontParams()
            }

            function updateFontParams() {

                var fontHeight = kcm.gridUnit * 0.4
                // use the ratio of the font height to the frame width, because there is more empty space vertically
                var newFontRatio = paintedWidth > 0 ? fontHeight / paintedWidth : 0.01
                // limit it so that nothing overlaps
                newFontRatio = Math.min(0.03, newFontRatio)
                // increase the font thickness because the image will shrink
                var thickness = paintedHeight > 0 && nativeHeight > 0 ? Math.round(nativeHeight / paintedHeight - 0.5) : 1

                kcm.previewProducer.setFontParams(newFontRatio, thickness)

                prevPaintedHeight = paintedHeight
                prevPaintedWidth = paintedWidth
            }
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

        Label {
            anchors.fill: parent
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: preview.message
        }
    }
}
