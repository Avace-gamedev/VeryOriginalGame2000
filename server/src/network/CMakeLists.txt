include_directories(${server_SOURCE_DIR}/include)

file(GLOB_RECURSE server_network_SRC "*.cpp")

add_library(server_network OBJECT ${server_network_SRC}) 
