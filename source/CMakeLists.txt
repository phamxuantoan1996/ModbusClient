cmake_minimum_required(VERSION 3.16)
project(Board_Extension_Modbus VERSION 1.0.0 LANGUAGES CXX)

# =========================
# C++ standard
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Library target
# =========================
add_library(ModbusClient STATIC
    modbusclient_rtu.cpp
)

add_library(ModbusClient::ModbusClient
    ALIAS ModbusClient
)

# =========================
# Include directories
# =========================
target_include_directories(ModbusClient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# =========================
# Dependencies
# =========================
find_library(LIBMODBUS_LIB modbus REQUIRED)
target_link_libraries(ModbusClient
    PUBLIC ${LIBMODBUS_LIB}
)

# =========================
# Install library
# =========================
install(TARGETS ModbusClient
    EXPORT ModbusClientTargets
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    DESTINATION include
)

# =========================
# Export targets
# =========================
install(EXPORT ModbusClientTargets
    FILE ModbusClientTargets.cmake
    NAMESPACE ModbusClient::
    DESTINATION lib/cmake/ModbusClient
)

# =========================
# Package config
# =========================
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ModbusClientConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)


configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ModbusClientConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ModbusClientConfig.cmake
    INSTALL_DESTINATION lib/cmake/ModbusClient
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ModbusClientConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ModbusClientConfigVersion.cmake
    DESTINATION lib/cmake/ModbusClient
)