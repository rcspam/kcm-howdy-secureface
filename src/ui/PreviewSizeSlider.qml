/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

// Size of the camera previews, shared by every page and remembered in
// ~/.config/kcm-howdyrc. Fixed width: it stays the same while the preview grows.
RowLayout {
    id: control

    // main.qml
    property var host
    // the label, when not given by a Kirigami.FormLayout
    property bool showLabel: true

    spacing: Kirigami.Units.smallSpacing

    QQC2.Label {
        visible: control.showLabel
        text: i18n("Preview size:")
    }
    QQC2.Slider {
        id: slider
        Layout.preferredWidth: Kirigami.Units.gridUnit * 10
        from: 0.5
        to: 3.0
        stepSize: 0.25
        snapMode: QQC2.Slider.SnapAlways
        value: control.host.previewZoom
        onMoved: control.host.previewZoom = value
    }
    QQC2.Label {
        // fixed width so the slider does not move while the number changes
        Layout.minimumWidth: metrics.width
        horizontalAlignment: Text.AlignRight
        text: i18nc("preview size in percent", "%1 %", Math.round(slider.value * 100))

        TextMetrics {
            id: metrics
            text: i18nc("preview size in percent", "%1 %", 300)
        }
    }
}
