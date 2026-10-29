cmake_minimum_required(VERSION 3.16.0)

project(kcm-howdy)

set(KF6_MIN_VERSION "6.3.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(HOWDY_CONFIG_FILE  "/etc/howdy/config.ini"            CACHE PATH "Howdy config file")
set(HOWDY_MODELS_PATH  "/etc/howdy/models"                CACHE PATH "Howdy models path")
set(DLIB_MODELS_PATH   "/usr/share/dlib-data"             CACHE PATH "Dlib models path")
set(HOWDY_COMMAND      "/usr/bin/howdy"                   CACHE FILEPATH "Howdy CLI run by the root helper")
set(MODULE_CONFIG_FILE "/etc/xdg/kcm-howdy/kcm-howdy.ini" CACHE PATH "Own module configuration file")

find_package(Qt6 CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Multimedia
)

find_package(KF6 REQUIRED COMPONENTS
    I18n
    KCMUtils
    Declarative
    Auth
)

find_package(Gettext REQUIRED)
find_package(OpenCV REQUIRED)
find_package(dlib REQUIRED)
find_package(lapack REQUIRED)
find_package(OpenBLAS REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Libsystemd REQUIRED IMPORTED_TARGET "libsystemd")

add_compile_options(-Werror=pedantic)
add_definitions(-DTRANSLATION_DOMAIN="kcm_howdy")

ecm_install_icons(ICONS
    sc-apps-howdy.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

add_subdirectory(src)

ki18n_install(po)
