/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents

// Face models of one account. main.qml has already pointed kcm.facesModel
// and the preview at this user before pushing the page.
KCM.ScrollViewKCM {
    id: page

    property var host
    property string userName
    property string realName
    property string avatar
    property bool isCurrentUser: false

    title: realName

    function addModel() {
        if (host.devicePath.length == 0) {
            host.closeAllTheBoxes()
            host.msgBox.show(i18n("No camera selected"), host.msgBox.iconError)
            return
        }
        kcm.push("AddFacePage.qml", { host: host, userName: userName, isCurrentUser: isCurrentUser })
    }

    function removeModel(row, label) {
        host.confirmBox.show(i18n("Delete the face model \"%1\"?\nThe operation cannot be undone.", label), host.confirmBox.iconWarning)
        host.confirmBox.acceptCallback = isCurrentUser
            ? () => kcm.facesModel.removeFace(row)
            : () => kcm.facesModel.removeOtherFace(userName, row)
    }

    function testModel(row) {
        kcm.push("TestFacePage.qml", {
            host: host,
            userName: userName,
            modelId: kcm.facesModel.getCell(row, 0),
            modelName: kcm.facesModel.getCell(row, 2),
        })
    }

    actions: [
        Kirigami.Action {
            id: addAction
            text: i18n("Add Face Model…")
            icon.name: "list-add"
            onTriggered: page.addModel()
        }
    ]

    header: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        KirigamiComponents.Avatar {
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            source: page.avatar
            name: page.realName
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Kirigami.Heading {
                Layout.fillWidth: true
                text: page.realName
                elide: Text.ElideRight
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: page.isCurrentUser ? i18nc("login name of the current user", "%1 (you)", page.userName) : page.userName
                opacity: 0.7
                elide: Text.ElideRight
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: !page.isCurrentUser
                text: i18n("Changes to another account need the administrator password.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                opacity: 0.7
            }
        }
    }

    view: ListView {
        id: facesList

        model: kcm.facesModel
        clip: true

        delegate: QQC2.ItemDelegate {
            id: faceDelegate

            required property int index

            readonly property string label: kcm.facesModel.getCell(index, 2)
            readonly property string date: kcm.facesModel.getCell(index, 1)

            width: ListView.view.width
            // the actions are the buttons, the row itself does nothing
            down: false
            hoverEnabled: false

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                    source: "user-identity"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: faceDelegate.label
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18n("Added %1", faceDelegate.date)
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                        elide: Text.ElideRight
                    }
                }

                QQC2.ToolButton {
                    icon.name: "media-playback-start"
                    text: i18n("Test")
                    display: QQC2.AbstractButton.TextBesideIcon
                    onClicked: page.testModel(faceDelegate.index)
                }
                QQC2.ToolButton {
                    icon.name: "edit-delete"
                    text: i18n("Delete")
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: page.removeModel(faceDelegate.index, faceDelegate.label)
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.largeSpacing * 4
            visible: facesList.count == 0
            icon.name: "user-identity"
            text: i18n("No face model yet")
            explanation: page.isCurrentUser
                         ? i18n("Add a face model to unlock the screen by looking at the camera.")
                         : i18n("Add a face model so this account can be unlocked with its face.")
            helpfulAction: addAction
        }
    }
}
