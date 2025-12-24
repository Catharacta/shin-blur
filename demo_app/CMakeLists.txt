cmake_minimum_required(VERSION 3.20)
project(blur_demo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows SDK
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0.22000.0)
endif()

# Demo application
add_executable(blur_demo WIN32
    main.cpp
    resource.rc
)

# Link to blur_lib
target_link_libraries(blur_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/../blur_lib/build/Release/blur_lib.lib
    comctl32
)

# Include blur_lib headers
target_include_directories(blur_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/../blur_lib/include
)

# Copy DLL to output directory
add_custom_command(TARGET blur_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/../blur_lib/build/Release/blur_lib.dll"
        $<TARGET_FILE_DIR:blur_demo>
)
