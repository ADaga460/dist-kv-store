include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
# Don't let gtest override the parent project's compiler/runtime settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(kv_tests
    test_store.cpp
    test_protocol.cpp
    test_config.cpp
    test_account.cpp
)
target_link_libraries(kv_tests PRIVATE kvcore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(kv_tests DISCOVERY_MODE PRE_TEST)
