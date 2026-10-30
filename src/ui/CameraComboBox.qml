/*
    SPDX-FileCopyrightText: 2024-2025 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import org.kde.kirigami as Kirigami

Row {
    id: node

    property var setValue
    property var model
    property var maxPopupWidth
    property var maxComboWidth
    property alias currentValue: combo.currentValue

    // only a choice made by the user, not the index set while the model loads
    signal activated(string value)

    function getCurrentItemProperty(name) {
        return combo.model.getItemProperty(combo.currentIndex, name)
    }

    ComboBox {
        id: combo

        property var maxItemWidth: 0

        // returns -1 if not found; "count" and "revision" make it run again once the
        // model is filled or scanned again. The config may name the camera by
        // another path than the list does.
        currentIndex: {
            count
            kcm.camModel.revision
            return Math.max(indexOfValue(kcm.camModel.matchingName(node.setValue)), 0)
        }
        onActivated: node.activated(currentValue)
        model: node.model
        valueRole: 'name'
        textRole: 'description'

        width: maxComboWidth ? Math.min(implicitWidth, maxComboWidth - refreshButton.width - warningIcon.width - node.spacing * 3) : implicitWidth

        delegate: ItemDelegate {

            property var infrared: model.infrared

            // the view is gone while a new camera list replaces the delegates
            width: ListView.view ? ListView.view.width : 0

            contentItem: Label {
                id: label
                text: description + (infrared ? " - " + i18n("IR camera, recommended") : "")
                elide: Text.ElideRight
            }
            Component.onCompleted: combo.maxItemWidth = Math.max(combo.maxItemWidth, implicitWidth)

            highlighted: combo.highlightedIndex === index
            ToolTip.text: label.text
            ToolTip.visible: highlighted && width > 0 && width < implicitWidth
            ToolTip.delay: 500

        }
        popup.width: Math.min(combo.maxItemWidth, node.maxPopupWidth)
    }
    // the list also follows cameras plugged in or out, this is the fallback
    ToolButton {
        id: refreshButton
        anchors.verticalCenter: parent.verticalCenter
        icon.name: "view-refresh"
        text: i18n("Look for cameras again")
        display: AbstractButton.IconOnly
        onClicked: kcm.camModel.refresh()
        ToolTip.text: text
        ToolTip.visible: hovered
        ToolTip.delay: Kirigami.Units.toolTipDelay
    }
    Kirigami.Icon {
        id: warningIcon

        width: kcm.gridUnit
        height: width
        anchors.verticalCenter: parent.verticalCenter
        visible: {
            var result = combo.model.getItemProperty(combo.currentIndex, 'infrared')
            return result !== undefined && !result
        }
        source: "data-warning"

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            ToolTip.visible: containsMouse
            ToolTip.text: i18n("Non IR camera, not recommended")
        }
    }
}
