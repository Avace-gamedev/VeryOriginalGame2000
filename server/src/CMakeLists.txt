include_directories(${server_SOURCE_DIR}/include)

add_executable(server server.cpp 
                $<TARGET_OBJECTS:server_common>
                $<TARGET_OBJECTS:server_network>
              )
target_link_libraries(server loguru ws2_32)

add_custom_command(TARGET server 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:server> ${PROJECT_BINARY_DIR})