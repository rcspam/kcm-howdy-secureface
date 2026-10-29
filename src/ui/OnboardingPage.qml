/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kcmutils as KCM

// First run wizard, shown while Howdy has no models folder yet.
KCM.AbstractKCM {
    id: page

    property var host
    // OnboardingView talks to "root", the id it had inside main.qml
    property var root: host

    title: i18n("Setup")

    // OnboardingView checks StackLayout.isCurrentItem before reacting to faces model signals
    StackLayout {
        anchors.fill: parent

        OnboardingView {}
    }
}
