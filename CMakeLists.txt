cmake_minimum_required(VERSION 4.0.2)
project(AIBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-w)

find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL Found: ${OPENSSL_FOUND}")
message(STATUS "OpenSSL Include Dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")

set(DPP_BUILD_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(external/dpp)

add_executable(AIBot
    src/main.cpp
    src/serviceHandler.cpp
    src/characterBot.cpp
    src/audioReceiver.cpp
    src/speechToText.cpp
)

target_include_directories(AIBot PRIVATE 
    /mingw64/include    
    include
    external/dpp/include
    external/leopard/include
)

target_link_libraries(AIBot PRIVATE ${CMAKE_SOURCE_DIR}/external/leopard/lib/libpv_leopard.lib)

find_package(CURL REQUIRED)
target_link_libraries(AIBot PRIVATE CURL::libcurl)

target_link_libraries(AIBot PRIVATE 
    stdc++ 
    stdc++fs
)

target_link_libraries(AIBot PRIVATE 
    dpp 
    ws2_32 
    winmm
)

add_custom_command(TARGET AIBot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dpp>
        $<TARGET_FILE_DIR:AIBot>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/include/leopard/languageModel.pv
        $<TARGET_FILE_DIR:AIBot>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/include/leopard/libpv_leopard.dll
        $<TARGET_FILE_DIR:AIBot>

    #COMMAND ${CMAKE_COMMAND} -E copy_if_different /mingw64/bin/libcrypto-3-x64.dll $<TARGET_FILE_DIR:AIBot>
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different /mingw64/bin/libssl-3-x64.dll $<TARGET_FILE_DIR:AIBot>
)