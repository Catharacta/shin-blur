cmake_minimum_required(VERSION 3.20)
project(blur_lib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows SDK version
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0.22000.0)
endif()

# Source files
set(BLUR_LIB_SOURCES
    src/blur_lib.cpp
    src/composition.cpp
    src/dwm_fallback.cpp
    src/d2d_blur.cpp
    src/error.cpp
    src/logging.cpp
    src/window_tracker.cpp

)

# Header files
set(BLUR_LIB_HEADERS
    include/blur_lib.h
)

# Create DLL
add_library(blur_lib SHARED ${BLUR_LIB_SOURCES} ${BLUR_LIB_HEADERS})

# Define export macro
target_compile_definitions(blur_lib PRIVATE BLUR_LIB_EXPORTS)

# Include directories
target_include_directories(blur_lib 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Windows libraries
if(WIN32)
    target_link_libraries(blur_lib PRIVATE
        user32
        dwmapi
        d2d1
        d3d11
        dxgi
        windowscodecs
    )
endif()

# Set output name
set_target_properties(blur_lib PROPERTIES
    OUTPUT_NAME "blur_lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Install rules
install(TARGETS blur_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES include/blur_lib.h
    DESTINATION include
)

# Testing
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
