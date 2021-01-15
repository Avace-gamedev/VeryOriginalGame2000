include_directories(${server_SOURCE_DIR}/include)
add_library(server_network network_frame.cpp network.cpp)
