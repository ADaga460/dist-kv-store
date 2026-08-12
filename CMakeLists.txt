cmake_minimum_required(VERSION 3.20)
project(dist_kv_store LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# On MinGW, statically link the C++/gcc/pthread runtimes so the binaries (and the
# test executable CTest launches during discovery) don't depend on toolchain DLLs
# being on PATH.
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

option(KV_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

# ---- Dependencies -----------------------------------------------------------

# standalone ASIO (header-only, no Boost)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
if(WIN32)
    # ASIO still needs the winsock libs to link, but we no longer touch the API directly.
    target_link_libraries(asio INTERFACE ws2_32 mswsock)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0601)
endif()
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)

# spdlog (bundles fmt)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

# ---- Core library -----------------------------------------------------------

add_library(kvcore STATIC
    src/protocol.cpp
    src/store.cpp
    src/config.cpp
    src/account.cpp
    src/net/connection.cpp
)
target_include_directories(kvcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kvcore PUBLIC asio spdlog::spdlog)

# ---- Executables ------------------------------------------------------------

add_executable(server src/server.cpp)
target_link_libraries(server PRIVATE kvcore)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE kvcore)

# ---- Tests ------------------------------------------------------------------

if(KV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
