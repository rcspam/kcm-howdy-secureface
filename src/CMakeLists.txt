kcoreaddons_add_plugin(kcm_howdy INSTALL_NAMESPACE "plasma/kcms/systemsettings")

target_sources(kcm_howdy PRIVATE kcm_howdy.cpp kcm_config.cpp facesmodel.cpp)

target_link_libraries(kcm_howdy
    Qt5::Core
    KF5::Auth
    KF5::CoreAddons
    KF5::I18n
    KF5::QuickAddons
)

kpackage_install_package(package kcm_howdy kcms)
kcmutils_generate_desktop_file(kcm_howdy)

add_executable(kcm-howdy-auth-helper auth_helper.cpp auth_helper.h)
target_link_libraries(kcm-howdy-auth-helper KF5::Auth KF5::ConfigCore KF5::I18n)
install(TARGETS kcm-howdy-auth-helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(kcm-howdy-auth-helper org.kde.kcontrol.kcmhowdy root)
kauth_install_actions(org.kde.kcontrol.kcmhowdy kcm_howdy.actions)
