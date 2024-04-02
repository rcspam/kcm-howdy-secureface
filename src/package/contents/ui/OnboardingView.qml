/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: onboadingView

    property bool wantPreview: views.currentIndex == 0 && camCombo.currentValue.length != 0
                            || views.currentIndex == 1 && camCombo.currentValue.length != 0 && viewScanFacePreview.message.length == 0

    Label {
        Layout.alignment: Qt.AlignCenter
        Layout.margins: kcm.gridUnit

        text: i18n("Face authentication setup wizard")
    }

    StackLayout {
        id: views

        ColumnLayout {
            id: viewSelectCamera

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.bottomMargin: kcm.gridUnit

                wrapMode: Text.WordWrap
                text: i18n("Please select a camera. It is highly recommended to use an IR camera for authentication.")
            }

            Item {
                Layout.fillWidth: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.bottomMargin: root.gap

                height: childrenRect.height

                CameraComboBox {
                    id: camCombo

                    x: viewSelectCameraPreview.x
                    spacing: root.gap
                    model: kcm.camModel.emptyItemProxy
                    maxWidth: viewSelectCamera.width - x - kcm.gridUnit
                    onCurrentValueChanged: kcm.previewProducer.setVideoPath(currentValue)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.maximumHeight: children[0].defaultHeight

                Preview {
                    id: viewSelectCameraPreview

                    // fit into the available space but not more than the default values, maintain the proportions
                    anchors.horizontalCenter: parent.horizontalCenter

                    property real maxWidth: Math.min(defaultWidth, parent.width)
                    property real maxHeight: Math.min(defaultHeight, parent.height)
                    readonly property real ratio: defaultHeight / defaultWidth
                    property real tall: maxHeight / maxWidth > ratio

                    width: tall ? maxWidth : Math.round(maxHeight / ratio)
                    height: tall ? Math.round(maxWidth * ratio) : maxHeight
                    enabled: visible && onboardingView.wantPreview
                }
            }

            function begin() {
                buttonBack.enabled = false
                buttonForward.enabled = true
                buttonForward.state = ""
            }

            function end() {
                if (camCombo.currentValue.length == 0) {
                    root.msgBox.show(i18n("You need to choose a camera."), root.msgBox.iconError)
                    return false
                }
                if (!camCombo.getCurrentItemProperty("infrared")) {
                    root.confirmBox.show(i18n("Using a non-IR camera for facial recognition is highly discouraged. For example, someone will be able to use your photo for successful authentication. Are you sure you want to proceed?"), root.confirmBox.iconWarning)
                    root.confirmBox.acceptCallback = () => {
                        if (updateCameraSettings()) views.nextNow()
                    }
                    return false
                }
                return updateCameraSettings()
            }

            function updateCameraSettings() {
                root.cfg_device.value = camCombo.currentValue
                return kcm.config.save()
            }
        }

        ColumnLayout {
            id: viewScanFace

            readonly property int face_not_scanned: 0
            readonly property int face_scanning: 1
            readonly property int face_scanned: 2

            property int faceScanStatus: face_not_scanned

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.bottomMargin: kcm.gridUnit

                wrapMode: Text.WordWrap
                text: i18n("Your face needs to be scanned to create a reference model. During authentication, a check will be made against this model.")
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.maximumHeight: children[0].defaultHeight

                Preview {
                    id: viewScanFacePreview

                    // fit into the available space but not more than the default values, maintain the proportions
                    anchors.horizontalCenter: parent.horizontalCenter

                    property real maxWidth: Math.min(defaultWidth, parent.width)
                    property real maxHeight: Math.min(defaultHeight, parent.height)
                    readonly property real ratio: defaultHeight / defaultWidth
                    property real tall: maxHeight / maxWidth > ratio

                    width: tall ? maxWidth : Math.round(maxHeight / ratio)
                    height: tall ? Math.round(maxWidth * ratio) : maxHeight
                    enabled: visible && onboardingView.wantPreview
                }
            }

            Button {
                id: scanButton

                Layout.topMargin: root.gap
                Layout.alignment: Qt.AlignHCenter

                text: i18n("Scan")
                enabled: viewScanFace.faceScanStatus == viewScanFace.face_not_scanned

                onClicked: {
                    viewScanFace.faceScanStatus = viewScanFace.face_scanning
                    viewScanFacePreview.message = i18n("Authorization")
                    kcm.facesModel.addFace("")
                }
            }

            function begin() {
                buttonBack.enabled = faceScanStatus == face_not_scanned
                buttonForward.enabled = faceScanStatus == face_scanned
                buttonForward.state = ""
            }

            function end() {
                return true
            }

            Connections {
                target: kcm.facesModel

                function onAuthorizedForAddFace() {
                    if (!viewScanFacePreview.visible) return
                    viewScanFacePreview.message = i18n("Look into the camera please")
                }

                function onSuccessAddFace() {
                    if (!viewScanFacePreview.visible) return
                    viewScanFacePreview.message = i18n("Face model added successfully")
                    viewScanFace.faceScanStatus = viewScanFace.face_scanned
                    buttonForward.enabled = true
                    buttonBack.enabled = false
                }

                function onErrorCreateFacesModel(message) {
                    viewScanFacePreview.message = ""
                    viewScanFace.faceScanStatus = viewScanFace.face_not_scanned
                }

                function onErrorAction(message) {
                    viewScanFacePreview.message = ""
                    viewScanFace.faceScanStatus = viewScanFace.face_not_scanned
                }
            }
        }

        ColumnLayout {
            id: viewSelectMode

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.bottomMargin: kcm.gridUnit

                wrapMode: Text.WordWrap
                text: i18n("Please select an operating mode:")
            }

            component RadioButtonDescribed : RadioButton {
                id: control

                required property string name
                required property string description

                contentItem: Column {

                    Label {
                        id: name
                        text: control.name
                        font.weight: 'Bold'
                    }

                    Label {
                        id: description
                        // it may affect the height of other views, while invisible
                        text: visible ? control.description : ""
                        wrapMode: Text.WordWrap
                        width: control.width - x
                        verticalAlignment: Text.AlignTop
                    }
                    leftPadding: control.indicator.width + control.spacing
                }
            }

            RadioButtonDescribed {
                id: radioFast

                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.fillWidth: true

                name: i18n("Fast")
                description: i18n("Allows more fuzzy matches, but speeds up the scanning process greatly.")
            }
            RadioButtonDescribed {
                id: radioBalanced

                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.fillWidth: true

                checked: true

                name: i18n("Balanced")
                description: i18n("Still relatively quick detection, but might not log you in when furher away.")
            }
            RadioButtonDescribed {
                id: radioSecure

                Layout.leftMargin: kcm.gridUnit
                Layout.rightMargin: kcm.gridUnit
                Layout.fillWidth: true

                name: i18n("Secure")
                description: i18n("The slightly safer option, but will take much longer to authenticate you.")
            }

            function begin() {
                buttonBack.enabled = true
                buttonForward.state = "finish"
            }

            function end() {
                var certainty = 5.0
                if (radioFast.checked) certainty = 4.2
                else if (radioBalanced.checked) certainty = 3.5
                else if (radioSecure.checked) certainty = 2.2

                root.cfg_certainty.value = certainty
                kcm.config.save()

                root.msgBox.show(i18n("The setup process is complete. Please note that you may need to additionally configure PAM to use facial recognition for authentication."), root.confirmBox.iconInfo)
                root.msgBox.closeCallback = () => {
                    root.enableOnboarding = false
                    views.nextNow()
                }
                return false
            }
        }

        function currentItem() {
            return count > 0 ? data[currentIndex] : undefined
        }

        function nextNow() {
            if (currentIndex + 1 < count) {
                currentIndex++;
                currentItem().begin()
            }
        }

        function next() {
            if (currentItem().end()) nextNow()
        }

        function back() {
            if (currentIndex > 0) {
                currentIndex--
                currentItem().begin()
            }
        }

        Component.onCompleted: {
            if (count > 0) data[0].begin()
        }
    }

    RowLayout {
        Layout.margins: kcm.gridUnit

        Button {
            id: buttonBack

            text: i18n("Back")
            onClicked: views.back()
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: buttonForward

            text: i18n("Next")
            onClicked: views.next()
            states: [
                State {
                    name: "finish"
                    PropertyChanges { target: buttonForward; text: i18n("Finish") }
                }
            ]
        }
    }
}
