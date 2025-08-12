cmake_minimum_required(VERSION 3.20)
project(axle_chain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch nlohmann/json (header-only)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch standalone Asio (header-only)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

# doctest for tests
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

add_library(axle_lib
    src/base58.cpp
    src/crypto.cpp
    src/encoding.cpp
    src/storage.cpp
    src/tx.cpp
    src/block.cpp
    src/ledger.cpp
    src/blockchain.cpp
    src/miner.cpp
    src/p2p.cpp
    src/rpc.cpp
    src/cli.cpp
)
target_include_directories(axle_lib PUBLIC include)
target_include_directories(axle_lib PRIVATE ${asio_SOURCE_DIR}/asio/include)
target_link_libraries(axle_lib PUBLIC nlohmann_json::nlohmann_json ${SODIUM_LIBRARIES})
target_compile_definitions(axle_lib PRIVATE ASIO_STANDALONE)
target_include_directories(axle_lib PRIVATE ${SODIUM_INCLUDE_DIRS})
target_link_directories(axle_lib PRIVATE ${SODIUM_LIBRARY_DIRS})

add_executable(axle src/main.cpp)
target_link_libraries(axle PRIVATE axle_lib)

enable_testing()
add_executable(axle_tests tests/tests.cpp)
target_link_libraries(axle_tests PRIVATE axle_lib doctest::doctest)
add_test(NAME unit COMMAND axle_tests)
