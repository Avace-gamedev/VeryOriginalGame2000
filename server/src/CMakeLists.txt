include_directories(${server_SOURCE_DIR}/include)

add_library(tinyxml2 STATIC IMPORTED)
set_target_properties(tinyxml2 PROPERTIES IMPORTED_LOCATION ${server_SOURCE_DIR}/lib/Debug/x64/tinyxml2.lib)

add_executable(server server.cpp 
                $<TARGET_OBJECTS:server_common>
                $<TARGET_OBJECTS:server_network>
                $<TARGET_OBJECTS:server_engine>
              )
target_link_libraries(server loguru tinyxml2 ws2_32)

add_custom_command(TARGET server 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:server> ${PROJECT_BINARY_DIR})