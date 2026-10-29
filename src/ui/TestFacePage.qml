/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Live recognition against one face model of one user
KCM.SimpleKCM {
    id: page

    property var host
    property string userName
    property var modelId
    property string modelName

    title: i18n("Test: %1", modelName)

    Component.onCompleted: {
        kcm.previewProducer.setUserName(userName)
        kcm.previewProducer.selectModel(modelId)
    }
    Component.onDestruction: kcm.previewProducer.unSelectModel()

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Look at the camera: a recognized face is marked with its certainty. The lower the value, the closer the match.")
        }

        PreviewSizeSlider {
            Layout.alignment: Qt.AlignHCenter
            host: page.host
        }

        Preview {
            Layout.alignment: Qt.AlignHCenter
            host: page.host
            availableWidth: page.width - Kirigami.Units.gridUnit * 2
            enabled: visible && host.devicePath.length != 0
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Done")
            icon.name: "dialog-ok"
            onClicked: kcm.pop()
        }
    }
}
