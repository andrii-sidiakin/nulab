add_executable(mine 
    tcp_client.c
    main.c
)
target_link_libraries(mine PRIVATE bclib)
