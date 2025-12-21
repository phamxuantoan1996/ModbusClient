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

