/*
    SPDX-FileCopyrightText: 2023-2025 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents
import org.altlinux.howdy.kcm.configmodule 1.0 // ConfigModule

// Laid out like the Users page of System Settings: the accounts in a sidebar,
// the selected account (or the shared camera settings) in the next column.
// Pushed pages do not see the ids of this file, they get it as "host".
KCM.ScrollViewKCM {
    id: root

    sidebarMode: true

    // Working copy of the settings. Pages read and write these properties,
    // the ConfigValue objects forward them to kcm.config, so a page that is
    // closed and opened again still shows the values not applied yet.
    property string devicePath: ""
    property int darkThreshold: 60
    property real certainty: 3.5
    property int timeout: 4
    property int maxHeight: 320
    property int deviceFps: -1
    property bool useCnn: false
    property bool howdyDisabled: false
    property bool detectionNotice: false
    property bool timeoutNotice: true
    property bool noConfirmation: false
    property bool abortIfSsh: true
    property bool abortIfLidClosed: true
    property bool nodRequired: false

    // bumped on every load, for bindings that read kcm.config directly
    property int configRevision: 0

    property var cfg_device: ConfigValue {
        branch: "video/"
        name: "device_path"
        type: cfgString
        defaultValue: ""
        listenValue: root.devicePath
    }

    property var cfg_darkThreshold: ConfigValue {
        branch: "video/"
        name: "dark_threshold"
        type: cfgInteger
        defaultValue: 60
        listenValue: root.darkThreshold
    }

    property var cfg_certainty: ConfigValue {
        branch: "video/"
        name: "certainty"
        type: cfgReal
        defaultValue: 3.5
        listenValue: root.certainty.toFixed(1)
    }

    property var cfg_timeout: ConfigValue {
        branch: "video/"
        name: "timeout"
        type: cfgInteger
        defaultValue: 4
        listenValue: root.timeout
    }

    property var cfg_maxHeight: ConfigValue {
        branch: "video/"
        name: "max_height"
        type: cfgInteger
        defaultValue: 320
        listenValue: root.maxHeight
    }

    property var cfg_deviceFps: ConfigValue {
        branch: "video/"
        name: "device_fps"
        type: cfgInteger
        defaultValue: -1
        listenValue: root.deviceFps
    }

    property var cfg_useCnn: ConfigValue {
        branch: "core/"
        name: "use_cnn"
        type: cfgBoolean
        defaultValue: false
        listenValue: root.useCnn
    }

    property var cfg_disabled: ConfigValue {
        branch: "core/"
        name: "disabled"
        type: cfgBoolean
        defaultValue: false
        listenValue: root.howdyDisabled
    }

    property var cfg_detectionNotice: ConfigValue {
        branch: "core/"
        name: "detection_notice"
        type: cfgBoolean
        defaultValue: false
        listenValue: root.detectionNotice
    }

    property var cfg_timeoutNotice: ConfigValue {
        branch: "core/"
        name: "timeout_notice"
        type: cfgBoolean
        defaultValue: true
        listenValue: root.timeoutNotice
    }

    property var cfg_noConfirmation: ConfigValue {
        branch: "core/"
        name: "no_confirmation"
        type: cfgBoolean
        defaultValue: false
        listenValue: root.noConfirmation
    }

    property var cfg_abortIfSsh: ConfigValue {
        branch: "core/"
        name: "abort_if_ssh"
        type: cfgBoolean
        defaultValue: true
        listenValue: root.abortIfSsh
    }

    property var cfg_abortIfLidClosed: ConfigValue {
        branch: "core/"
        name: "abort_if_lid_closed"
        type: cfgBoolean
        defaultValue: true
        listenValue: root.abortIfLidClosed
    }

    // runs the stamp rules of config.ini, offered only when they are a single nod
    property var cfg_nodRequired: ConfigValue {
        branch: "rubberstamps/"
        name: "enabled"
        type: cfgBoolean
        defaultValue: false
        listenValue: root.nodRequired
    }

    // the setup wizard writes these two ConfigValues directly
    Connections {
        target: root.cfg_device
        function onValueChanged() { root.devicePath = root.cfg_device.value }
    }
    Connections {
        target: root.cfg_certainty
        function onValueChanged() { root.certainty = root.cfg_certainty.value }
    }

    function loadWorkingCopy() {
        devicePath = cfg_device.value
        darkThreshold = cfg_darkThreshold.value
        certainty = cfg_certainty.value
        timeout = cfg_timeout.value
        maxHeight = cfg_maxHeight.value
        deviceFps = cfg_deviceFps.value
        useCnn = cfg_useCnn.value
        howdyDisabled = cfg_disabled.value
        detectionNotice = cfg_detectionNotice.value
        timeoutNotice = cfg_timeoutNotice.value
        noConfirmation = cfg_noConfirmation.value
        abortIfSsh = cfg_abortIfSsh.value
        abortIfLidClosed = cfg_abortIfLidClosed.value
        nodRequired = cfg_nodRequired.value
        configRevision++

        // The handlers below only fire on a change, and a reload usually brings
        // the same values: hand them to the capture explicitly, or it keeps a
        // dark threshold of 0, shows the dark IR frames and never looks for faces.
        kcm.previewProducer.setVideoPath(devicePath)
        kcm.previewProducer.setDarkThreshold(darkThreshold)
        kcm.previewProducer.setCertainty(certainty)
    }

    function setPreviewUser(userName) {
        if (userName) kcm.previewProducer.setUserName(userName)
    }

    readonly property string currentUserName: kcm.usersModel.get(kcm.usersModel.currentUserIndex).name || ""

    SystemPalette { id: paletteActive;   colorGroup: SystemPalette.Active }
    SystemPalette { id: paletteInactive; colorGroup: SystemPalette.Inactive }
    property var gap: Math.round(kcm.gridUnit / 3)

    // Size of the camera previews, per user and remembered between sessions
    Settings {
        id: uiSettings
        location: StandardPaths.writableLocation(StandardPaths.GenericConfigLocation) + "/kcm-howdyrc"
        category: "Preview"
        property real zoom: 1.5
    }
    property alias previewZoom: uiSettings.zoom

    // Every visible Preview counts itself in, capture runs while one is shown.
    // Scanning a face hides the preview, which frees the camera for howdy.
    property int previewUsers: 0
    property bool captureError: false
    onPreviewUsersChanged: kcm.previewProducer.capturing = previewUsers > 0

    onDevicePathChanged: kcm.previewProducer.setVideoPath(devicePath)
    onDarkThresholdChanged: kcm.previewProducer.setDarkThreshold(darkThreshold)
    onCertaintyChanged: kcm.previewProducer.setCertainty(certainty)

    property string errorString: kcm.errorString
    property bool enableOnboarding: kcm.enableOnboarding

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

    // "settings" or the login of the selected user
    property string selected: ""

    function showPage(fileName, properties) {
        while (kcm.depth > 1) {
            kcm.pop()
        }
        properties.host = root
        kcm.push(fileName, properties)
    }

    function openSettings() {
        selected = "settings"
        setDefaultsButtonVisibility(true)
        // the settings preview matches faces against the logged-in user
        setPreviewUser(currentUserName)
        showPage("SettingsPage.qml", {})
    }

    function openUser(row) {
        const user = kcm.usersModel.get(row)
        if (!user.name) return
        selected = user.name
        setDefaultsButtonVisibility(false)
        // load the models before the page binds to them
        kcm.facesModel.setUserName(user.name)
        kcm.previewProducer.setUserName(user.name)
        showPage("UserPage.qml", {
            userName: user.name,
            realName: user.realName,
            avatar: user.avatar,
            isCurrentUser: row === kcm.usersModel.currentUserIndex,
        })
    }

    onEnableOnboardingChanged: {
        if (!enableOnboarding) Qt.callLater(openUser, kcm.usersModel.currentUserIndex)
    }

    Component.onCompleted: {
        kcm.columnWidth = Kirigami.Units.gridUnit * 15
        // the capture refuses to start without a user, the setup wizard needs it too
        setPreviewUser(currentUserName)
        Qt.callLater(() => {
            if (errorString.length > 0) return
            if (enableOnboarding) {
                showPage("OnboardingPage.qml", {})
            } else {
                openUser(kcm.usersModel.currentUserIndex)
            }
        })
    }

    view: ListView {
        id: usersList

        readonly property bool ready: root.errorString.length == 0 && !root.enableOnboarding

        model: ready ? kcm.usersModel : null
        clip: true

        header: ColumnLayout {
            width: ListView.view.width
            spacing: 0
            visible: usersList.ready

            QQC2.ItemDelegate {
                Layout.fillWidth: true
                highlighted: root.selected === "settings"
                text: i18n("Camera and recognition")
                icon.name: "camera-web"
                onClicked: root.openSettings()
            }

            Kirigami.ListSectionHeader {
                Layout.fillWidth: true
                text: i18n("Users")
            }
        }

        delegate: QQC2.ItemDelegate {
            id: userDelegate

            required property int index
            required property string name
            required property string realName
            required property string avatar
            required property int modelCount

            width: ListView.view.width
            highlighted: root.selected === name
            onClicked: root.openUser(index)

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                KirigamiComponents.Avatar {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                    source: userDelegate.avatar
                    name: userDelegate.realName
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: userDelegate.realName
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: userDelegate.modelCount > 0
                              ? i18np("%1 face model", "%1 face models", userDelegate.modelCount)
                              : i18n("No face model")
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.largeSpacing * 4
            visible: !usersList.ready
            icon.name: root.errorString.length > 0 ? "dialog-error" : "user-identity"
            text: root.errorString.length > 0 ? root.errorString : i18n("Set up facial authentication first")
        }
    }

    // Centred on the window: the sidebar this file lives in is too narrow
    property var msgBox: MsgBox {
        parent: QQC2.Overlay.overlay
        property var closeCallback
        onClosed: {
            if (closeCallback) closeCallback()
            closeCallback = null
        }
    }

    property var msgBoxNoButton: MsgBox {
        parent: QQC2.Overlay.overlay
        footer: QQC2.DialogButtonBox {}
    }

    property var confirmBox: MsgBox {
        parent: QQC2.Overlay.overlay
        buttons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
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

        function onConfigLoaded() {
            configValue.setupConfigValues(root)
            loadWorkingCopy()
        }

        function onSetDefaults() {
            configValue.setupConfigValues(root, null, true)
            loadWorkingCopy()
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
            msgBox.closeCallback = () => root.captureError = true
        }
    }

    Connections {
        target: kcm

        function onErrorInitBackend(message) {
            errorString = i18n("Error init backend:") + "\n" + message
        }
    }
}
