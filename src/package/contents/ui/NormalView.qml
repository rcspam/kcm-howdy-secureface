/*
    SPDX-FileCopyrightText: 2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Column {
    id: normalView

    property var wantPreview: camCombo.currentValue.length != 0 && bar.currentIndex == 0 && modelsOrPreview.currentIndex == 1 && preScanPreview.message.length == 0
                              || camCombo.currentValue.length != 0 && bar.currentIndex == 1 && enablePreview.checked


    property alias devicePath: camCombo.currentValue
    property alias darkThreshold: sliderDarkThreshold.value
    property alias certainty: sliderCertainty.value

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
                    root.configValue.setupConfigValues(root)
                }
            } else {
                prevIndex = currentIndex
            }
            setDefaultsButtonVisibility(currentIndex != 0)
        }
    }

    StackLayout {
        id: stack
        width: parent.width
        height: parent.height - bar.height
        currentIndex: bar.currentIndex

        StackLayout {
            id: modelsOrPreview

            Column {
                padding: root.gap
                spacing: root.gap

                Column {
                    id: modelTable
                    clip: true
                    visible: kcm.facesModel.rows > 0

                    HorizontalHeaderView {
                        id: facesTableHeader
                        syncView: facesTable
                        columnWidthProvider: facesTable.columnWidthProvider
                        boundsBehavior: Flickable.StopAtBounds
                    }

                    ScrollView {
                        clip: true
                        visible: kcm.facesModel.rows > 0
                        height: Math.min(stack.height - modelButtons.height - y - root.gap, implicitHeight)

                        Component.onCompleted: {
                            background.visible = true
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    leftPadding: root.gap * 2
                                    rightPadding: root.gap * 2
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
                    padding: root.gap * 2
                }
                Row {
                    id: modelButtons
                    spacing: root.gap

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
            ScrollView {
                Column {
                    padding: root.gap
                    spacing: root.gap
                    Preview {
                        id: preScanPreview
                        enabled: visible && normalView.wantPreview
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
                        spacing: root.gap
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
        }
        ScrollView {
            id: deviceTab
            Column {
                padding: root.gap
                spacing: root.gap

                CameraComboBox {
                    id: camCombo
                    spacing: root.gap
                    model: kcm.camModel.emptyItemProxy
                    setValue: root.cfg_device.value
                    maxWidth: deviceTab.availableWidth - x * 2
                    onCurrentValueChanged: kcm.previewProducer.setVideoPath(currentValue)
                }

                Grid {
                    columns: 3
                    spacing: root.gap

                    Text {
                        text: i18n("Darkness threshold")
                    }
                    Slider {
                        id: sliderDarkThreshold
                        from: 0.0
                        to: 100.0
                        stepSize: 1.0
                        snapMode: Slider.SnapAlways
                        value: root.cfg_darkThreshold.value
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
                        value: root.cfg_certainty.value
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
                    enabled: visible && normalView.wantPreview
                }
            }
        }
    }

    Connections {
        target: kcm.facesModel

        function onAuthorizedForAddFace() {
            if (!preScanPreview.visible) return
            preScanPreview.message = i18n("Look into the camera please")
        }

        function onSuccessAddFace() {
            if (!preScanPreview.visible) return
            modelsOrPreview.currentIndex = 0
            closeAllTheBoxes()
            msgBox.show(i18n("Face model added successfully"), msgBox.iconInfo)
        }
    }
}
