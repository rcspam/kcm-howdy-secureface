/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Settings shared by every account. Widgets read and write the working copy
// in main.qml (host), the Apply button of System Settings saves it.
KCM.SimpleKCM {
    id: page

    property var host

    title: i18n("Camera and Recognition")

    // Settings that weaken or break the unlock on this setup: shown, never edited here
    function lockedValue(url, fallback) {
        host.configRevision // re-read after every load
        const value = kcm.config.getConfigValue(url)
        return value === undefined || value === "" ? fallback : value
    }
    readonly property string workaround: lockedValue("core/workaround", "off")
    readonly property string saveFailed: lockedValue("snapshots/save_failed", "false")
    readonly property string saveSuccessful: lockedValue("snapshots/save_successful", "false")
    readonly property string rubberstamps: lockedValue("rubberstamps/enabled", "false")
    readonly property string recordingPlugin: lockedValue("video/recording_plugin", "opencv")
    readonly property bool lockedRisk: workaround !== "off" || saveFailed === "true" || saveSuccessful === "true"
                                       || rubberstamps === "true" || recordingPlugin !== "opencv"

    // unchecking it stops the camera and its IR emitter
    property bool showPreview: true

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.FormLayout {
            id: cameraForm
            Layout.fillWidth: true
            twinFormLayouts: [settingsForm]

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Camera")
            }

            CameraComboBox {
                Kirigami.FormData.label: i18n("Device:")
                spacing: Kirigami.Units.smallSpacing
                model: kcm.camModel.emptyItemProxy
                setValue: page.host.devicePath
                maxComboWidth: Kirigami.Units.gridUnit * 24
                maxPopupWidth: page.width - Kirigami.Units.gridUnit * 2
                // not onCurrentValueChanged: while loading, the combo passes through
                // its empty first entry, which would clear the device of the config
                onActivated: (value) => page.host.devicePath = value
            }

            QQC2.CheckBox {
                text: i18n("Show the camera preview")
                checked: page.showPreview && !page.host.captureError
                onToggled: {
                    page.showPreview = checked
                    if (checked) page.host.captureError = false
                }
            }

            PreviewSizeSlider {
                Kirigami.FormData.label: i18n("Preview size:")
                visible: page.showPreview
                host: page.host
                showLabel: false
            }
        }

        // outside the forms: a wide preview would overflow their field column
        Preview {
            Layout.alignment: Qt.AlignHCenter
            host: page.host
            visible: page.showPreview
            availableWidth: page.width - Kirigami.Units.gridUnit * 2
            enabled: visible && page.host.devicePath.length != 0
        }

        Kirigami.FormLayout {
            id: settingsForm
            Layout.fillWidth: true
            twinFormLayouts: [cameraForm]

            RowLayout {
                Kirigami.FormData.label: i18n("Darkness threshold:")
                spacing: Kirigami.Units.smallSpacing

                QQC2.Slider {
                    id: darkSlider
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                    from: 0
                    to: 100
                    stepSize: 1
                    snapMode: QQC2.Slider.SnapAlways
                    value: page.host.darkThreshold
                    onMoved: page.host.darkThreshold = value
                }
                QQC2.Label {
                    text: darkSlider.value
                }
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                text: i18n("Frames with a larger share of dark pixels than this percentage are skipped.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                opacity: 0.7
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Recognition")
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Certainty:")
                spacing: Kirigami.Units.smallSpacing

                QQC2.Slider {
                    id: certaintySlider
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                    from: 0
                    // Howdy advises against more than 5
                    to: 5
                    stepSize: 0.1
                    snapMode: QQC2.Slider.SnapAlways
                    value: page.host.certainty
                    onMoved: page.host.certainty = value
                }
                QQC2.Label {
                    text: certaintySlider.value.toFixed(1)
                }
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                text: i18n("Lower is stricter. Howdy advises against values above 5.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                opacity: 0.7
            }
            Kirigami.InlineMessage {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                Layout.fillWidth: true
                visible: certaintySlider.value > 4
                type: Kirigami.MessageType.Warning
                text: i18n("Above 4, another person's face is more likely to be accepted.")
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Search for:")
                from: 1
                to: 15
                value: page.host.timeout
                textFromValue: (value, locale) => i18np("%1 second", "%1 seconds", value)
                valueFromText: (text, locale) => parseInt(text)
                onValueModified: page.host.timeout = value
            }

            QQC2.CheckBox {
                text: i18n("Use the CNN face detector")
                checked: page.host.useCnn
                onToggled: page.host.useCnn = checked
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                text: i18n("More accurate, but much slower than the default detector.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                opacity: 0.7
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Analysis height:")
                from: 120
                to: 720
                stepSize: 40
                value: page.host.maxHeight
                textFromValue: (value, locale) => i18nc("height in pixels", "%1 px", value)
                valueFromText: (text, locale) => parseInt(text)
                onValueModified: page.host.maxHeight = value
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                text: i18n("Taller frames are scaled down before analysis. Lower is faster.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.WordWrap
                opacity: 0.7
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Frame rate:")
                textRole: "text"
                valueRole: "value"
                model: {
                    const choices = [
                        { text: i18n("Highest available"), value: -1 },
                        { text: i18nc("frames per second", "%1 fps", 30), value: 30 },
                        { text: i18nc("frames per second", "%1 fps", 15), value: 15 },
                    ]
                    if (![-1, 30, 15].includes(page.host.deviceFps)) {
                        choices.push({ text: i18nc("frames per second, value set by hand", "%1 fps (custom)", page.host.deviceFps), value: page.host.deviceFps })
                    }
                    return choices
                }
                currentIndex: Math.max(indexOfValue(page.host.deviceFps), 0)
                onActivated: page.host.deviceFps = currentValue
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("General")
            }

            QQC2.CheckBox {
                Kirigami.FormData.label: i18n("Facial authentication:")
                text: i18n("Disable for every account")
                checked: page.host.howdyDisabled
                onToggled: page.host.howdyDisabled = checked
            }
            Kirigami.InlineMessage {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                Layout.fillWidth: true
                visible: page.host.howdyDisabled
                type: Kirigami.MessageType.Information
                text: i18n("The face models are kept. Unlocking asks for the password only.")
            }

            QQC2.CheckBox {
                Kirigami.FormData.label: i18n("Messages:")
                text: i18n("Announce the face scan")
                checked: page.host.detectionNotice
                onToggled: page.host.detectionNotice = checked
            }
            QQC2.CheckBox {
                text: i18n("Tell when no face was recognized in time")
                checked: page.host.timeoutNotice
                onToggled: page.host.timeoutNotice = checked
            }
            QQC2.CheckBox {
                text: i18n("Show who was recognized")
                checked: !page.host.noConfirmation
                onToggled: page.host.noConfirmation = !checked
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Safety")
            }

            QQC2.CheckBox {
                Kirigami.FormData.label: i18n("Skip facial authentication:")
                text: i18n("In SSH sessions")
                checked: page.host.abortIfSsh
                onToggled: page.host.abortIfSsh = checked
            }
            Kirigami.InlineMessage {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                Layout.fillWidth: true
                visible: !page.host.abortIfSsh
                type: Kirigami.MessageType.Warning
                text: i18n("Someone connected remotely could then be let in by whoever sits in front of the camera.")
            }
            QQC2.CheckBox {
                text: i18n("When the lid is closed")
                checked: page.host.abortIfLidClosed
                onToggled: page.host.abortIfLidClosed = checked
            }
            Kirigami.InlineMessage {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                Layout.fillWidth: true
                visible: !page.host.abortIfLidClosed
                type: Kirigami.MessageType.Information
                text: i18n("With the lid closed the camera sees nothing: every attempt waits for the search time before the password is asked.")
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Locked Settings")
            }

            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                text: i18n("These settings can only be changed in Howdy's config.ini, as root, because they weaken or break facial unlock here.")
                wrapMode: Text.WordWrap
                opacity: 0.7
            }
            Kirigami.InlineMessage {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 24
                Layout.fillWidth: true
                visible: page.lockedRisk
                type: Kirigami.MessageType.Warning
                text: i18n("At least one of them has a risky value. Check Howdy's config.ini.")
            }

            LockedSetting {
                Kirigami.FormData.label: i18n("Password workaround:")
                value: page.workaround
                risky: page.workaround !== "off"
                reason: i18n("Must stay off: \"input\" and \"native\" let a typed password through unchecked with some PAM setups.")
            }
            LockedSetting {
                Kirigami.FormData.label: i18n("Snapshots:")
                value: page.saveFailed === "true" || page.saveSuccessful === "true" ? i18n("on") : i18n("off")
                risky: page.saveFailed === "true" || page.saveSuccessful === "true"
                reason: i18n("Must stay off: the lock screen cannot write them and facial unlock would fail.")
            }
            LockedSetting {
                Kirigami.FormData.label: i18n("Rubber stamps:")
                value: page.rubberstamps === "true" ? i18n("on") : i18n("off")
                risky: page.rubberstamps === "true"
                reason: i18n("Must stay off: a stamp that fails with an error counts as a success.")
            }
            LockedSetting {
                Kirigami.FormData.label: i18n("Capture backend:")
                value: page.recordingPlugin
                risky: page.recordingPlugin !== "opencv"
                reason: i18n("OpenCV is the backend that works with this camera, the others easily break capture.")
            }
        }
    }
}
