/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: node

    property var setValue
    property var model
    property var maxWidth
    property alias currentValue: combo.currentValue

    function getCurrentItemProperty(name) {
        return combo.model.getItemProperty(combo.currentIndex, name)
    }

    ComboBox {
        id: combo

        property var maxItemWidth: 0

        // returns -1 if not found
        currentIndex: Math.max(indexOfValue(node.setValue), 0)
        model: node.model
        valueRole: 'name'
        textRole: 'description'

        delegate: ItemDelegate {

            property var infrared: model.infrared

            width: ListView.view.width
            contentItem: Text {
                id: label
                text: description + (infrared ? " - " + i18n("IR camera, recommended") : "")
            }
            Component.onCompleted: combo.maxItemWidth = Math.max(combo.maxItemWidth, implicitWidth)

            highlighted: combo.highlightedIndex === index
            ToolTip.text: label.text
            ToolTip.visible: highlighted && width < implicitWidth
            ToolTip.delay: 500
        }
        popup.width: Math.min(combo.maxItemWidth, node.maxWidth)
    }
    Image {
        width: kcm.gridUnit
        height: width
        anchors.verticalCenter: parent.verticalCenter
        visible: {
            var result = combo.model.getItemProperty(combo.currentIndex, 'infrared')
            return result !== undefined && !result
        }
        source: "image://icon/data-warning"

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            ToolTip.visible: containsMouse
            ToolTip.text: i18n("Non IR camera, not recommended")
        }
    }
}
