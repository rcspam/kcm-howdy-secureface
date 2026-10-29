/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

// One read-only line of the "Locked Settings" section: current value and why
// it cannot be changed from here.
ColumnLayout {
    id: item

    property string value
    property string reason
    property bool risky: false

    spacing: 0
    Layout.maximumWidth: Kirigami.Units.gridUnit * 24

    RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            source: item.risky ? "data-warning" : "object-locked"
        }
        QQC2.Label {
            text: item.value
            font.bold: item.risky
            color: item.risky ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
        }
    }
    QQC2.Label {
        Layout.fillWidth: true
        text: item.reason
        font: Kirigami.Theme.smallFont
        wrapMode: Text.WordWrap
        opacity: 0.7
    }
}
