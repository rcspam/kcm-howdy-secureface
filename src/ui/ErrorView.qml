/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: errorView

    Label {
        id: errorMsg
        anchors.centerIn: parent
        text: root.errorString
    }
}
