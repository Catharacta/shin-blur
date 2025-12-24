enable_testing()

# Simple test executable
add_executable(test_blur_lib test_blur.cpp)
target_link_libraries(test_blur_lib PRIVATE blur_lib)
target_include_directories(test_blur_lib PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME BasicTest COMMAND test_blur_lib)
