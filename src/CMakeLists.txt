kcoreaddons_add_plugin(kcm_howdy INSTALL_NAMESPACE "plasma/kcms/systemsettings")

target_sources(kcm_howdy PRIVATE kcm_howdy.cpp kcm_config.cpp)

target_link_libraries(kcm_howdy
    Qt5::Core
    KF5::CoreAddons
    KF5::I18n
    KF5::QuickAddons
)

kpackage_install_package(package kcm_howdy kcms)
kcmutils_generate_desktop_file(kcm_howdy)
