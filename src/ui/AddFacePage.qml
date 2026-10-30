/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Scan a new face model: the preview helps placing the face, then it is
// hidden while howdy (run as root by the helper) takes over the camera.
KCM.SimpleKCM {
    id: page

    property var host
    property string userName
    property bool isCurrentUser: false

    title: i18n("Add Face Model")

    function scan() {
        preview.message = i18n("Authorization")
        modelName.enabled = false
        if (isCurrentUser) {
            kcm.facesModel.addFace(modelName.text)
        } else {
            kcm.facesModel.addOtherFace(userName, modelName.text)
        }
    }

    // The back arrow of the columns only scrolls back and left the page open.
    // A running scan cannot be cancelled, like with the Cancel button.
    onBackRequested: (event) => {
        event.accepted = true
        if (modelName.enabled) kcm.pop()
    }
    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: page.visible && modelName.enabled
        onActivated: kcm.pop()
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        PreviewSizeSlider {
            Layout.alignment: Qt.AlignHCenter
            host: page.host
        }

        Preview {
            id: preview
            Layout.alignment: Qt.AlignHCenter
            host: page.host
            availableWidth: page.width - Kirigami.Units.gridUnit * 2
            enabled: visible && host.devicePath.length != 0 && message.length == 0
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.TextField {
                id: modelName
                Kirigami.FormData.label: i18n("Model name:")
                placeholderText: i18n("For example: with glasses")
                maximumLength: 24
                Keys.onReturnPressed: page.scan()
                Keys.onEnterPressed: page.scan()
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            enabled: modelName.enabled
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                text: i18n("Scan")
                icon.name: "camera-photo"
                onClicked: page.scan()
            }
            QQC2.Button {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onClicked: kcm.pop()
            }
        }
    }

    Connections {
        target: kcm.facesModel

        function onAuthorizedForAddFace() {
            preview.message = i18n("Look into the camera please")
        }

        function onSuccessAddFace() {
            kcm.pop()
            host.closeAllTheBoxes()
            host.msgBox.show(i18n("Face model added successfully"), host.msgBox.iconInfo)
        }

        function onErrorAction(message) {
            preview.message = ""
            modelName.enabled = true
        }
    }
}
