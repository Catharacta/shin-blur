enable_testing()

# Simple test executable
add_executable(test_blur_lib test_blur.cpp)
target_link_libraries(test_blur_lib PRIVATE blur_lib)
target_include_directories(test_blur_lib PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME BasicTest COMMAND test_blur_lib)

# Benchmark executable
add_executable(blur_benchmark benchmark.cpp)
target_link_libraries(blur_benchmark PRIVATE blur_lib)
target_include_directories(blur_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)
