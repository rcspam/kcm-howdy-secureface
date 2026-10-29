include(ECMAddTests)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Test)

ecm_add_test(usersfilter_test.cpp
    TEST_NAME usersfilter_test
    LINK_LIBRARIES Qt6::Core Qt6::Test
)
target_include_directories(usersfilter_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
