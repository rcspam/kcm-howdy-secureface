/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia 5.15

import org.kde.kcm 1.4 as KCM

KCM.SimpleKCM {
    id: root

    SystemPalette { id: paletteActive;   colorGroup: SystemPalette.Active }
    SystemPalette { id: paletteInactive; colorGroup: SystemPalette.Inactive }
    property var gap: Math.round(kcm.gridUnit / 3)

    // one general condition for all instances of preview
    // so that capture restarts do not occur
    property var previewEnabled: camCombo.currentValue.length != 0 && bar.currentIndex == 0 && modelsOrPreview.currentIndex == 1 && preScanPreview.message.length == 0
                                 || camCombo.currentValue.length != 0 && bar.currentIndex == 1 && enablePreview.checked

    property var cfg_device: ConfigValue {
        branch: "video/"
        name: "device_path"
        type: cfgString
        defaultValue: ""
        listenValue: camCombo.currentValue
    }

    property var cfg_darkThreshold: ConfigValue {
        branch: "video/"
        name: "dark_threshold"
        type: cfgInteger
        defaultValue: 60
        listenValue: sliderDarkThreshold.value
    }

    property var cfg_certainty: ConfigValue {
        branch: "video/"
        name: "certainty"
        type: cfgReal
        defaultValue: 3.5
        listenValue: sliderCertainty.value.toFixed(1)
    }

    property string errorString: ""

    Item {
        id: normalView
        anchors.fill: parent
        visible: errorString.length == 0
        TabBar {
            id: bar
            width: parent.width
            property int prevIndex: currentIndex
            TabButton {
                text: i18n("Model list")
            }
            TabButton {
                text: i18n("Device")
            }
            onCurrentIndexChanged: {
                if (currentIndex == 0 && prevIndex != 0 && kcm.needsSave) {
                    currentIndex = prevIndex
                    confirmBox.show(i18n("Settings changes will be lost."), confirmBox.iconWarning)
                    confirmBox.acceptCallback = () => {
                        prevIndex = 0
                        currentIndex = 0
                        configValue.setupConfigValues(root)
                    }
                } else {
                    prevIndex = currentIndex
                }
            }
        }
        StackLayout {
            anchors.top: bar.bottom
            width: parent.width
            currentIndex: bar.currentIndex

            StackLayout {
                id: modelsOrPreview

                Column {
                    padding: gap
                    spacing: gap

                    ScrollView {
                        id: scrollView
                        clip: true
                        visible: kcm.facesModel.rows > 0

                        Component.onCompleted: {
                            background.visible = true
                        }

                        Column {
                            HorizontalHeaderView {
                                id: facesTableHeader
                                syncView: facesTable
                                columnWidthProvider: facesTable.columnWidthProvider
                                boundsBehavior: Flickable.StopAtBounds
                            }

                            TableView {
                                id: facesTable
                                width: columnWidths.reduce((acc, cur) => acc + cur, 0)
                                boundsBehavior: Flickable.StopAtBounds
                                height: contentHeight
                                model: kcm.facesModel
                                property var columnWidths: [1, 3 * kcm.gridUnit, 8 * kcm.gridUnit]
                                property var selectedRow: -1
                                columnWidthProvider: function (column) { return columnWidths[column] }

                                delegate: Item {
                                    id: rootDelegate

                                    Rectangle {
                                        id: hoverRect
                                        x: {
                                            var pos = 0
                                            for (var i = 0; i < column; ++i) {
                                                pos += facesTable.columnWidths[i]
                                            }
                                            return -pos
                                        }
                                        width: facesTable.width
                                        height: parent.height
                                        color: "transparent"
                                        border {
                                            width: 1
                                            color: paletteActive.highlight
                                        }
                                        visible: cellArea.containsMouse
                                    }
                                    Rectangle {
                                        id: selectedRect
                                        anchors.fill: rootDelegate
                                        color: paletteActive.highlight
                                        visible: row == facesTable.selectedRow
                                    }
                                    Text {
                                        id: myText
                                        anchors.verticalCenter: parent.verticalCenter
                                        leftPadding: gap * 2
                                        rightPadding: gap * 2
                                        text: display
                                        color: row == facesTable.selectedRow ? paletteActive.highlightedText : paletteActive.text
                                        Component.onCompleted: {
                                            facesTable.columnWidths[column] = Math.max(facesTable.columnWidths[column], implicitWidth)
                                            facesTable.columnWidthsChanged()
                                            kcm.facesModel.layoutChanged()
                                        }
                                    }
                                    MouseArea {
                                        id: cellArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: facesTable.selectedRow = row
                                    }
                                }
                            }
                        }
                    }
                    Text {
                        text: i18n("No face models")
                        visible: kcm.facesModel.rows == 0
                        padding: gap * 2
                    }
                    Row {
                        spacing: gap

                        Button {
                            text: i18n("Add")
                            onClicked: {
                                if (camCombo.currentValue.length == 0) {
                                    closeAllTheBoxes()
                                    msgBox.show(i18n("No camera selected"), msgBox.iconError)
                                    return
                                }
                                newModelName.text = ""
                                newModelName.enabled = true
                                preScanPreview.message = ""
                                modelsOrPreview.currentIndex = 1
                            }
                        }
                        Button {
                            text: i18n("Delete")
                            visible: kcm.facesModel.rows > 0
                            enabled: facesTable.selectedRow >= 0
                            onClicked: {
                                confirmBox.show(i18n("Are you sure you want to delete this model?\nThe operation cannot be undone."), confirmBox.iconWarning)
                                confirmBox.acceptCallback = () => kcm.facesModel.removeFace(facesTable.selectedRow)
                            }
                        }
                    }
                }
                Column {
                    padding: gap
                    spacing: gap
                    Preview {
                        id: preScanPreview
                        enabled: previewEnabled
                    }
                    Text {
                        wrapMode: Text.WordWrap
                        enabled: newModelName.enabled
                        text: i18n("Enter a name for the new model")
                    }

                    TextField {
                        id: newModelName
                        Keys.onReturnPressed: scanButton.onClicked()
                    }
                    Row {
                        enabled: newModelName.enabled
                        spacing: gap
                        Button {
                            id: scanButton
                            text: i18n("Scan")
                            onClicked: {
                                preScanPreview.message = i18n("Authorization")
                                newModelName.enabled = false
                                kcm.facesModel.addFace(newModelName.text)
                            }
                        }
                        Button {
                            text: i18n("Cancel")
                            onClicked: modelsOrPreview.currentIndex = 0
                        }
                    }
                }
            }
            Column {
                padding: gap
                spacing: gap

                ComboBox {
                    id: camCombo
                    // returns -1 if not found
                    currentIndex: Math.max(indexOfValue(cfg_device.value), 0)
                    model: kcm.camModel
                    valueRole: 'name'
                    textRole: 'description'
                    onCurrentValueChanged: kcm.previewProducer.setVideoPath(currentValue)
                }
                Grid {
                    columns: 3
                    spacing: gap

                    Text {
                        text: i18n("Darkness threshold")
                    }
                    Slider {
                        id: sliderDarkThreshold
                        from: 0.0
                        to: 100.0
                        stepSize: 1.0
                        snapMode: Slider.SnapAlways
                        value: cfg_darkThreshold.value
                        onValueChanged: kcm.previewProducer.setDarkThreshold(this.value)
                    }
                    Text {
                        text: sliderDarkThreshold.value
                    }
                    Text {
                        text: i18n("Certainty")
                    }
                    Slider {
                        id: sliderCertainty
                        from: 0.0
                        to: 15.0
                        stepSize: 0.1
                        snapMode: Slider.SnapAlways
                        value: cfg_certainty.value
                        onValueChanged: kcm.previewProducer.setCertainty(this.value)
                    }
                    Text {
                        text: sliderCertainty.value.toFixed(1)
                    }
                }
                CheckBox {
                    id: enablePreview
                    text: i18n("Enable preview")
                    checked: true
                }
                Preview {
                    enabled: previewEnabled
                }
            }
        }
    }

    // if as a result of an error we cannot do anything at all
    Item {
        id: errorView
        visible: errorString.length != 0
        anchors.fill: parent

        Label {
            id: errorMsg
            anchors.centerIn: parent
            text: errorString
        }
    }

    MsgBox {
        id: msgBox
        property var closeCallback
        onClosed: {
            if (closeCallback) closeCallback()
            closeCallback = null
        }
    }

    MsgBox {
        id: msgBoxNoButton
        footer: DialogButtonBox {}
    }

    MsgBox {
        id: confirmBox
        buttons: Dialog.Ok | Dialog.Cancel
        property var acceptCallback
        onAccepted: {
            if (acceptCallback) acceptCallback()
            acceptCallback = null
        }
    }

    function closeAllTheBoxes() {
        msgBox.reject()
        msgBoxNoButton.reject()
        confirmBox.reject()
        modelsOrPreview.currentIndex = 0
    }

    ConfigValue {
        id: configValue
    }

    Connections {
        target: kcm.config

        function onConfigLoaded() {
            configValue.setupConfigValues(root)
        }

        function onErrorLoadConfigFile(filename) {
            errorString = i18n("Error load config file:") + "\n" + filename
        }

        function onErrorSaveConfigFile(message) {
            closeAllTheBoxes()
            msgBox.show(i18n("Save settings failed:") + "\n" + message, msgBox.iconError)
        }

        function onSetDefaults() {
            configValue.setupConfigValues(root, null, true)
        }
    }

    Connections {
        target: kcm.facesModel

        function onErrorCreateFacesModel(message) {
            errorString = i18n("Error create faces model:") + "\n" + message
        }

        function onErrorAction(message) {
            closeAllTheBoxes()
            msgBox.show(message, msgBox.iconError)
        }

        function onSuccessAddFace() {
            closeAllTheBoxes()
            msgBox.show(i18n("Face model added successfully"), msgBox.iconInfo)
        }

        function onAuthorizedForAddFace() {
            preScanPreview.message = i18n("Look into the camera please")
        }
    }

    Connections {
        target: kcm.previewProducer

        function onCaptureErrorOccurred(errorString) {
            closeAllTheBoxes()
            msgBox.show(i18n("Capture error occurred:") + "\n" + errorString, msgBox.iconError)
            msgBox.closeCallback = () => enablePreview.checked = false
        }
    }

    Connections {
        target: kcm

        function onErrorInitBackend(message) {
            errorString = i18n("Error init backend:") + "\n" + message
        }
    }
}
