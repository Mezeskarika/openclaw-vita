cmake_minimum_required(VERSION 3.2)

# Host-side unit tests for engine code that makes no SDL calls.
# Only the bundled SDL headers are used, so no SDL library has to be installed.
project(OpenClawTests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../ThirdParty/)

add_executable(openclaw_tests
    TestMain.cpp
    GamepadMapperTests.cpp
    DifficultyTests.cpp
    ../OpenClaw/Engine/UserInterface/GamepadMapper.cpp
)

enable_testing()
add_test(NAME openclaw_tests COMMAND openclaw_tests)
