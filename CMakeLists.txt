cmake_minimum_required(VERSION 3.14)  # Increased minimum version for FetchContent

project(LLVMExamplePass)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(include)

add_library(passes MODULE
  lib/ExamplePass.cpp
  lib/Pluto/BogusControlFlowPass.cpp
  lib/Pluto/CryptoUtils.cpp
  lib/Pluto/Flattening.cpp
  lib/Pluto/GlobalEncryption.cpp
  lib/Pluto/IndirectCall.cpp
  lib/Pluto/MBAUtils.cpp
  lib/PassRegistration.cpp
)

# Fetch GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test executable
add_executable(tests tests/tests.cpp)
target_link_libraries(tests GTest::gtest_main)

# Add test to CTest
add_test(NAME tests COMMAND tests)

# Custom command to run the test
add_custom_target(run_test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tests
  DEPENDS tests passes
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running tests"
)

