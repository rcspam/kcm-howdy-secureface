kcmutils_add_qml_kcm(kcm_howdy)

target_sources(kcm_howdy PRIVATE
    kcm_howdy.cpp
    kcm_config.cpp
    facesmodel.cpp
    cammodel.cpp
    usersmodel.cpp
    qpixmapitem.cpp
    )

add_subdirectory(previewproducer)

target_link_libraries(kcm_howdy
    Qt6::Core
    Qt6::Multimedia
    Qt6::Quick
    KF6::AuthCore
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtilsQuick
    PkgConfig::Libsystemd
    previewproducer
)

get_filename_component(MODULE_CONFIG_DIR  ${MODULE_CONFIG_FILE} DIRECTORY)
get_filename_component(MODULE_CONFIG_NAME ${MODULE_CONFIG_FILE} NAME)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/kcm-howdy.ini DESTINATION ${MODULE_CONFIG_DIR} RENAME ${MODULE_CONFIG_NAME})

add_executable(kcm-howdy-auth-helper auth_helper.cpp auth_helper.h)
target_link_libraries(kcm-howdy-auth-helper KF6::AuthCore KF6::ConfigCore KF6::I18n)
install(TARGETS kcm-howdy-auth-helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(kcm-howdy-auth-helper org.kde.kcontrol.kcmhowdy root)
kauth_install_actions(org.kde.kcontrol.kcmhowdy kcm_howdy.actions)
