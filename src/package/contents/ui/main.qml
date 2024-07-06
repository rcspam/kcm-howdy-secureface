/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia 5.15

import org.kde.kcm 1.4 as KCM
import org.altlinux.howdy.kcm.configmodule 1.0 // ConfigModule

KCM.AbstractKCM {
    id: root

    property var cfg_device: ConfigValue {
        branch: "video/"
        name: "device_path"
        type: cfgString
        defaultValue: ""
        listenValue: normalView.devicePath
    }

    property var cfg_darkThreshold: ConfigValue {
        branch: "video/"
        name: "dark_threshold"
        type: cfgInteger
        defaultValue: 60
        listenValue: normalView.darkThreshold
    }

    property var cfg_certainty: ConfigValue {
        branch: "video/"
        name: "certainty"
        type: cfgReal
        defaultValue: 3.5
        listenValue: normalView.certainty.toFixed(1)
    }

    SystemPalette { id: paletteActive;   colorGroup: SystemPalette.Active }
    SystemPalette { id: paletteInactive; colorGroup: SystemPalette.Inactive }
    property var gap: Math.round(kcm.gridUnit / 3)

    // activate video capture centrally
    property var previewEnabled: onboardingView.wantPreview || normalView.wantPreview
    onPreviewEnabledChanged: {
        // enabled can be undefined
        kcm.previewProducer.capturing = previewEnabled ? true : false
    }

    property string errorString: ""
    property bool enableOnboarding: true

    function setDefaultsButtonVisibility(visible) {
        if (visible) {
            kcm.buttons |= ConfigModule.Default
        } else {
            kcm.buttons &= ~ConfigModule.Default
        }
        // unfortunately I didn't find a better way
        // to reach the function ModuleView::updateButtons() in systemsettings app
        // see also:
        // https://bugs.kde.org/477718
        // https://discuss.kde.org/t/systemsettings-do-not-update-buttons-when-configmodule-buttons-property-is-changed/7946
        kcm.needsSave = !kcm.needsSave
        kcm.needsSave = !kcm.needsSave
    }

    property var msgBox: MsgBox {
        property var closeCallback
        onClosed: {
            if (closeCallback) closeCallback()
            closeCallback = null
        }
    }

    property var msgBoxNoButton: MsgBox {
        footer: DialogButtonBox {}
    }

    property var confirmBox: MsgBox {
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

    StackLayout {
        anchors.fill: parent

        currentIndex: {
            if (errorString.length > 0) return 0
            if (enableOnboarding) return 1
            return 2
        }

        ErrorView {}
        OnboardingView { id: onboardingView }
        NormalView { id: normalView }
    }

    property var configValue: ConfigValue {}

    Connections {
        target: kcm.config

        function onErrorLoadConfigFile(filename) {
            errorString = i18n("Error load config file:") + "\n" + filename
        }

        function onErrorSaveConfigFile(message) {
            closeAllTheBoxes()
            msgBox.show(i18n("Save settings failed:") + "\n" + message, msgBox.iconError)
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

    Connections {
        target: kcm.config

        function onConfigLoaded() {
            configValue.setupConfigValues(root)
        }

        function onSetDefaults() {
            configValue.setupConfigValues(root, null, true)
        }
    }
}
