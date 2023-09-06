cmake_minimum_required(VERSION 3.16.0)

project(kcm-howdy)

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.97.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(HOWDY_CONFIG_FILE "/etc/howdy/config.ini" CACHE PATH "Howdy config file")
set(HOWDY_MODELS_PATH "/etc/howdy/models"     CACHE PATH "Howdy models path")

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
)

find_package(KF5 REQUIRED COMPONENTS
    I18n
    KCMUtils
    Declarative
    Auth
)

find_package(Gettext REQUIRED)

add_compile_options(-Werror=pedantic)

ecm_install_icons(ICONS
    sc-apps-howdy.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

add_subdirectory(src)

ki18n_install(po)
