cmake_minimum_required(VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CXX_COMPILER_ID:MSVC>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(GameLeaderboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(GameLeaderboard
  src/main.cpp
  src/service/RankCalculator.cpp
)

target_include_directories(GameLeaderboard PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

enable_testing()

add_executable(RankCalculatorTest
    tests/RankCalculatorTest.cpp
    src/service/RankCalculator.cpp
)

target_include_directories(RankCalculatorTest
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_test(NAME RankCalculatorTest
    COMMAND RankCalculatorTest
)