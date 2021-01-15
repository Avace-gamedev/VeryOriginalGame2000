
include_directories(${server_SOURCE_DIR}/include)

file(GLOB_RECURSE server_engine_SRC "*.cpp")

add_library(server_engine OBJECT ${server_engine_SRC}) 
