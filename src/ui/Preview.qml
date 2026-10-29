/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.altlinux.howdy.kcm.custom 1.0 as Custom

Rectangle {
    id: preview

    property var enabled: false
    property string message: ""

    // main.qml, for the capture counter and the shared preview size
    property var host: null
    // width the parent view can give, 0 keeps the fixed default size
    property real availableWidth: 0

    // own copy, pushed pages do not see the one of main.qml
    property int gap: Math.round(kcm.gridUnit / 3)

    property int defaultWidth: kcm.gridUnit * 20 + gap * 2
    property int defaultHeight: kcm.gridUnit * 15 + gap * 2

    property int prevPaintedHeight: 0
    property int prevPaintedWidth: 0

    // sized by host.previewZoom (set with PreviewSizeSlider) rather than fixed
    readonly property bool zoomable: host !== null && availableWidth > 0
    // camera size once frames arrive, the IR sensor of this laptop until then
    readonly property real nativeFrameWidth: videoOut.nativeWidth > 0 ? videoOut.nativeWidth : 640
    readonly property real frameRatio: videoOut.nativeWidth > 0 && videoOut.nativeHeight > 0
                                       ? videoOut.nativeWidth / videoOut.nativeHeight : 16 / 9
    // 100 % shows one camera pixel per screen pixel, whatever the scale factor
    // availableWidth is 0 while the page is being created
    readonly property int frameWidth: Math.max(0, Math.round(Math.min(nativeFrameWidth / Screen.devicePixelRatio * (host ? host.previewZoom : 1),
                                                                      availableWidth - gap * 2)))

    // Implicit size: inside a layout, a plain width is overridden by the layout.
    // The setup wizard still sets width and height itself.
    implicitWidth: zoomable ? frameWidth + gap * 2 : defaultWidth
    implicitHeight: zoomable ? Math.round(frameWidth / frameRatio) + gap * 2 : defaultHeight

    radius: gap
    color: Kirigami.Theme.backgroundColor
    border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
    border.width: 1

    // Capture runs while at least one preview is shown
    readonly property bool active: preview.enabled === true && visible && host !== null && !host.captureError
    property bool counted: false

    function updateCount() {
        if (!host) return
        if (active && !counted) {
            host.previewUsers += 1
            counted = true
        } else if (!active && counted) {
            host.previewUsers -= 1
            counted = false
        }
    }

    onActiveChanged: updateCount()
    Component.onCompleted: updateCount()
    Component.onDestruction: if (counted && host) host.previewUsers -= 1

    Item {
        id: frameArea

        x: gap
        y: gap
        width: preview.width - gap * 2
        height: preview.height - gap * 2
        visible: message.length == 0

        Custom.QPixmapItem {
            id: videoOut
            anchors.fill: parent
            pixmap: kcm.previewProducer.pixmap
            smooth: true
            fillMode: Custom.QPixmapItem.PreserveAspectFit
            visible: preview.active && !kcm.previewProducer.waitingCamera

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
            visible: !preview.active
            color: "black"
        }

        QQC2.BusyIndicator {
            anchors.centerIn: parent
            running: visible
            visible: preview.active && kcm.previewProducer.waitingCamera
        }
    }

    Item {
        anchors.fill: frameArea
        visible: message.length != 0

        QQC2.Label {
            anchors.fill: parent
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: preview.message
        }
    }
}
