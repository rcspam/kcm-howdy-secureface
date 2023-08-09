/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

import QtQml 2.15

QtObject {

    // types - to correctly parse a string from the config
    property int cfgUnknown: 0
    property int cfgString:  1
    property int cfgInteger: 2
    property int cfgBoolean: 3

    property string branch
    property string name
    property var type: cfgUnknown
    property var defaultValue

    property var value: defaultValue // --> to widget
    property var listenValue         // <-- from widget
    property var storedValue

    function valueFromString(stringValue) {

        switch (type) {
            case cfgString:  value = stringValue; break;
            case cfgInteger: value = parseInt(stringValue); break;
            case cfgBoolean: value = stringValue === "true"; break;
            default: console.error("Type unknown for ConfigValue: (" + type + ") name: " + name + " branch: " + branch)
        }
    }

    onListenValueChanged: {
        if (!(listenValue || listenValue == "") || branch == "") return
        var parsed = listenValue.toString()
        kcm.config.updateConfigValue(branch + name, parsed)
        // without this check, you get a binding loop: value -> widget -> listenValue -> value ...
        if (value != listenValue) value = listenValue
    }

    function setupConfigValues(item, branch, setDefaults) {
        let props = []
        for (var p in item) {
            if (p.startsWith("cfg_") && !p.endsWith("Changed")) {
                if (branch) item[p].branch = branch
                props.push(p)
                if (setDefaults) {
                    item[p].value = item[p].defaultValue
                    continue
                }
                let configValue = kcm.config.getConfigValue(item[p].branch + item[p].name)
                if (configValue) {
                    item[p].valueFromString(configValue)
                } else {
                    item[p].value = item[p].defaultValue
                    // to keep track of future changes
                    kcm.config.storeDefaultValue(item[p].branch + item[p].name, item[p].defaultValue)
                }
                item[p].storedValue = item[p].value
            }
        }
        return props
    }
}
