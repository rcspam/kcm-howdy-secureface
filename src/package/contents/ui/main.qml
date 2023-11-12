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

    property var cfg_device: ConfigValue {
        branch: "video/"
        name: "device_path"
        type: cfgString
        defaultValue: ""
        listenValue: camCombo.currentValue
    }

    property var cfg_darkThreshold: ConfigValue {
        branch: "video/"
        name: "dark_treshold"
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

    property bool errorOccurred: false

    Item {
        id: normalView
        anchors.fill: parent
        visible: !errorOccurred
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
                        onClicked: inputModelName.show()
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
                    enabled: camCombo.currentValue != "" && bar.currentIndex == 1 && enablePreview.checked
                }
            }
        }
    }

    // if as a result of an error we cannot do anything at all
    Item {
        id: errorView
        visible: errorOccurred
        anchors.fill: parent

        Label {
            id: errorMsg
            anchors.centerIn: parent
            text: "PUT_ERROR_TEXT_HERE"
        }
    }

    Dialog {
        id: inputModelName

        // center manually so that there is an integer value
        x: Math.round((parent.width - width) * 0.5)
        y: Math.round((parent.height - height) * 0.5)

        property var optimalWidth: footer.minWidth + contentWidth
        property var width0: Math.min(optimalWidth, parent.width * 0.7, kcm.gridUnit * 20)

        width: Math.max(footer.minWidth, contentWidth + gap * 4, width0)
        height: width >= optimalWidth ? implicitHeight - footer.height + gap * 2 : implicitHeight

        function show() {
            value.text = ""
            open()
            value.forceActiveFocus()
        }

        verticalPadding: 0
        horizontalPadding: gap * 2
        clip: true

        header: Text {
            padding: gap * 2
            wrapMode: Text.WordWrap
            width: Math.min(implicitWidth, parent.width)
            text: i18n("Enter a name for the new model")
        }

        TextField {
            id: value
            Keys.onReturnPressed: inputModelName.accept()
        }

        footer: DialogButtonBox {
            padding: gap * 2
            topPadding: gap * 2
            property int minWidth: implicitWidth + spacing * 2
            standardButtons: Dialog.Ok | Dialog.Cancel
            buttonLayout: DialogButtonBox.KdeLayout
            alignment: Qt.AlignBottom | Qt.AlignRight
        }

        onAccepted: kcm.facesModel.addFace(value.text)
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
            errorMsg.text = i18n("Error load config file:") + "\n" + filename
            errorOccurred = true
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
            errorMsg.text = i18n("Error create faces model:") + "\n" + message
            errorOccurred = true
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
            msgBoxNoButton.show(i18n("Look into the camera please"))
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
            errorMsg.text = i18n("Error init backend:") + "\n" + message
            errorOccurred = true
        }
    }
}
