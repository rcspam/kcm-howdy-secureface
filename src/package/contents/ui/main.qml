/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    property var cfg_device: ConfigValue {
        branch: "video/"
        name: "device_path"
        type: cfgString
        defaultValue: ""
        listenValue: devicePath.text
    }

    property bool errorOccurred: false

    Item {
        id: normalView
        anchors.fill: parent
        visible: !errorOccurred
        TextField {
            id: devicePath
            text: cfg_device.value
        }
    }

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

    ConfigValue {
        id: configValue
    }

    Connections {
        target: kcm.config
        function onConfigLoaded() {
            configValue.setupConfigValues(root)
        }

        function onErrorLoadConfigFile(filename) {
            errorMsg.text = i18n("Error load config file:\n%1", filename)
            errorOccurred = true
        }
    }
}
