include_directories(${server_SOURCE_DIR}/include)
add_library(server_common OBJECT time.cpp utils.cpp bitarray.cpp)