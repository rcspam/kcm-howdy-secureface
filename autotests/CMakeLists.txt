include(ECMAddTests)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Test)

ecm_add_test(usersfilter_test.cpp
    TEST_NAME usersfilter_test
    LINK_LIBRARIES Qt6::Core Qt6::Test
)
target_include_directories(usersfilter_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(overlay_test.cpp
    TEST_NAME overlay_test
    LINK_LIBRARIES Qt6::Core Qt6::Test
)
target_include_directories(overlay_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(recognitionpacer_test.cpp
    TEST_NAME recognitionpacer_test
    LINK_LIBRARIES Qt6::Core Qt6::Test
)
target_include_directories(recognitionpacer_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(stamprules_test.cpp
    TEST_NAME stamprules_test
    LINK_LIBRARIES Qt6::Core Qt6::Test
)
target_include_directories(stamprules_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
