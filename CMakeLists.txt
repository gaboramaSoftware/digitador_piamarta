cmake_minimum_required(VERSION 3.15)
project(DigitadorPiamarta VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release) 

add_definitions(-DASIO_STANDALONE -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601)

include_directories(
    src
    src/h
    src/bd
    src/libs/include
    src/asio-1.30.2/include
)

set(SOURCES
    src/main.cpp
    src/cpp/DB_Backend.cpp
    src/cpp/Sensor.cpp
    src/cpp/SensorWorker.cpp
    src/cpp/HardwareManager.cpp
    src/cpp/TemplateManager.cpp
    src/cpp/HeadlessManager.cpp
    src/cpp/CrowServer.cpp
    src/cpp/Endpoints.cpp
    src/bd/sqlite3.c
)

add_executable(Digitador ${SOURCES})

target_link_libraries(Digitador PRIVATE 
    "${CMAKE_SOURCE_DIR}/src/libs/x64lib/libzkfp.lib"
    ws2_32
    mswsock
    pthread
)

if(MINGW)
    target_link_options(Digitador PRIVATE 
        "-static-libgcc" 
        "-static-libstdc++"
    )
endif()

add_custom_command(TARGET Digitador POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/src/libs/x64lib"
    "$<TARGET_FILE_DIR:Digitador>"
    COMMENT "Copiando DLLs del huellero..."
)