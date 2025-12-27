cmake_minimum_required(VERSION 3.16)
project(app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔹 Tìm thư viện bạn đã install
find_package(ModbusClient REQUIRED)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})

add_executable(app
    main.cpp
)

# 🔹 Link bằng target export (QUAN TRỌNG)
target_link_libraries(app
    PRIVATE ModbusClient::ModbusClient Threads::Threads ${JSONCPP_LIBRARIES}
)
