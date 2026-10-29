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
        }

        // Drawn here at screen resolution: drawn into the small IR frame,
        // the text and circles were blurred by the scaling.
        Item {
            id: overlayLayer

            readonly property var info: kcm.previewProducer.overlay
            readonly property var faces: info.faces || []
            // frame pixels to item pixels
            readonly property real ratio: info.frameWidth > 0 ? width / info.frameWidth : 0
            // statistics and histogram only when they do not cover the face
            readonly property bool roomy: height >= Kirigami.Units.gridUnit * 8
            readonly property int margin: Kirigami.Units.smallSpacing

            x: Math.round((videoOut.width - videoOut.paintedWidth) / 2)
            y: Math.round((videoOut.height - videoOut.paintedHeight) / 2)
            width: videoOut.paintedWidth
            height: videoOut.paintedHeight
            visible: videoOut.visible && ratio > 0
            clip: true

            function statistics() {
                // nothing to format before the first frame
                if (!(info.frameWidth > 0)) return ""
                var lines = []
                if (info.recognitionMs >= 0) lines.push(i18n("Recognition: %1 ms", info.recognitionMs))
                lines.push(i18n("Frames: %1", info.frames))
                lines.push(i18n("FPS: %1", info.fps))
                lines.push(i18n("Resolution: %1×%2", info.frameWidth, info.frameHeight))
                return lines.join("\n")
            }

            // the delegates stay while the number of faces does not change
            Repeater {
                model: overlayLayer.faces.length

                delegate: Item {
                    id: mark

                    required property int index
                    // the old delegates can still be evaluated once the list got shorter
                    readonly property var face: overlayLayer.faces[index] || { x: 0, y: 0, r: 0, state: "unchecked" }
                    readonly property color tint: face.state === "matched" ? Kirigami.Theme.positiveTextColor
                                                                           : Kirigami.Theme.negativeTextColor
                    readonly property real r: face.r * overlayLayer.ratio
                    readonly property real cx: face.x * overlayLayer.ratio
                    readonly property real cy: face.y * overlayLayer.ratio

                    anchors.fill: parent

                    Rectangle {
                        x: mark.cx - mark.r
                        y: mark.cy - mark.r
                        width: mark.r * 2
                        height: mark.r * 2
                        radius: mark.r
                        color: "transparent"
                        border.width: 2
                        border.color: mark.tint
                        antialiasing: true
                    }

                    OverlayTag {
                        visible: mark.face.state !== "unchecked"
                        color: mark.tint
                        text: mark.face.state === "matched" ? i18n("Model #%1, certainty %2", mark.face.model, mark.face.certainty.toFixed(2))
                            : mark.face.state === "nomatch" ? i18n("No match, certainty %1", mark.face.certainty.toFixed(2))
                            : i18n("Match failed")
                        // above the circle, slightly right of its center, kept inside the frame
                        x: Math.max(0, Math.min(mark.cx + mark.r / 3, overlayLayer.width - width))
                        y: Math.max(0, Math.min(mark.cy - mark.r - height - overlayLayer.margin, overlayLayer.height - height))
                    }
                }
            }

            OverlayTag {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: overlayLayer.margin
                color: overlayLayer.info.dark ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
                text: overlayLayer.info.dark ? i18n("Dark frame") : i18n("Scan frame")
            }

            // brightness histogram, darkest bin first: Howdy skips the frame
            // when the first bin goes over the dark threshold.
            // Same size on screen as the one the capture used to draw.
            Row {
                id: histBars

                readonly property real barsHeight: Math.round(Kirigami.Units.gridUnit * 10 / 3)

                visible: overlayLayer.roomy
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: overlayLayer.margin
                height: barsHeight

                Repeater {
                    model: (overlayLayer.info.hist || []).length

                    delegate: Rectangle {
                        required property int index
                        // no anchors: a delegate being destroyed has no parent left
                        y: histBars.barsHeight - height
                        width: Math.round(Kirigami.Units.gridUnit * 2 / 3)
                        height: Math.max(1, (overlayLayer.info.hist[index] || 0) / 100 * histBars.barsHeight)
                        color: Kirigami.Theme.positiveTextColor
                    }
                }
            }

            OverlayTag {
                visible: overlayLayer.roomy
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: overlayLayer.margin
                text: overlayLayer.statistics()
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

    // text on a dark backdrop, readable on any part of the frame
    component OverlayTag: QQC2.Label {
        font: Kirigami.Theme.smallFont
        color: Kirigami.Theme.positiveTextColor
        topPadding: 1
        bottomPadding: 1
        leftPadding: Kirigami.Units.smallSpacing
        rightPadding: Kirigami.Units.smallSpacing
        background: Rectangle {
            radius: Kirigami.Units.cornerRadius
            color: Qt.rgba(0, 0, 0, 0.6)
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
