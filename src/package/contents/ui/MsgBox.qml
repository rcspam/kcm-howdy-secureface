/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {

    // center manually so that there is an integer value
    x: Math.round((parent.width - width) * 0.5)
    y: Math.round((parent.height - height) * 0.5)

    property int rawContentWidth: messageIcon.width + measureText.width + gap * 2 + leftPadding + rightPadding

    property int width0: Math.min(rawContentWidth, parent.width * 0.8, kcm.gridUnit * 25)
    width: Math.max(width0, kcm.gridUnit * 12)

    property alias buttons: footerItem.standardButtons
    property string message: ""

    property string iconInfo:    "dialog-information"
    property string iconWarning: "dialog-warning"
    property string iconError:   "dialog-error"

    function show(msg, icon) {
        if (!icon || icon === "") icon = iconInfo
        messageIcon.source = "image://icon/" + icon
        message = msg
        open()
    }

    verticalPadding: 0
    horizontalPadding: gap * 2
    clip: true

    Label {
        id: measureText
        visible: false
        text: message
    }

    contentItem: Row {

        spacing: gap * 2

        Image {
            width: kcm.gridUnit * 2
            height: width
            id: messageIcon
            anchors.verticalCenter: parent.verticalCenter
            source: "image://icon/image-missing"
        }
        Label {
            id: messageItem
            wrapMode: Text.WordWrap
            width: parent.width - x
            anchors.verticalCenter: parent.verticalCenter
            text: message
        }
    }

    footer: DialogButtonBox {
        id: footerItem

        topPadding: 0
        bottomPadding: gap * 2
        standardButtons: { return DialogButtonBox.Ok }
        alignment: Qt.AlignHCenter
    }
}
