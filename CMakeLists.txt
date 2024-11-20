cmake_minimum_required(VERSION 3.15)
project(flight_booking)

# Set vcpkg toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    else()
        message(FATAL_ERROR "Could not find vcpkg toolchain file. Please set VCPKG_ROOT environment variable or install vcpkg in C:/vcpkg")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
set(unofficial-sqlite3_DIR "C:/vcpkg/installed/x64-windows/share/unofficial-sqlite3")
set(nlohmann_json_DIR "C:/vcpkg/installed/x64-windows/share/nlohmann_json")
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Add executable
add_executable(flight_booking 
    src/registration.cpp
)

# Include directories
target_include_directories(flight_booking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(flight_booking PRIVATE 
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(flight_booking PRIVATE ws2_32)
endif()

# Set output directories
set_target_properties(flight_booking PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")