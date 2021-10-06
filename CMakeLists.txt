cmake_minimum_required(VERSION 3.10)

project(walrus VERSION 0.1 LANGUAGES C CXX)

# -- Build Library --
file(GLOB_RECURSE Sources src/*.c)
message(STATUS "Building Walrus ...")

add_library(walrus STATIC ${Sources})
configure_file(include/walrus.configure.h.in include/walrus.configure.h)

target_include_directories(walrus PUBLIC include src/query/include)

# -- Build Shell --
add_executable(walrus-shell main.c)

target_link_libraries(walrus-shell PUBLIC walrus)
target_include_directories(walrus-shell PUBLIC include)