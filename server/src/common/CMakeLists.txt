include_directories(${server_SOURCE_DIR}/include)

file(GLOB_RECURSE server_common_SRC "*.cpp")

add_library(server_common OBJECT ${server_common_SRC})