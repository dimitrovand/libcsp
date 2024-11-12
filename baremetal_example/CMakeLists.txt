add_executable(server_client EXCLUDE_FROM_ALL server_client.c)
target_include_directories(server_client PRIVATE ${csp_inc})
target_link_libraries(server_client PRIVATE csp)

add_executable(server_ping EXCLUDE_FROM_ALL server_ping.c)
target_include_directories(server_ping PRIVATE ${csp_inc})
target_link_libraries(server_ping PRIVATE csp)

add_executable(server_rdp EXCLUDE_FROM_ALL server_rdp.c)
target_include_directories(server_rdp PRIVATE ${csp_inc})
target_link_libraries(server_rdp PRIVATE csp)
