
#
#
#

# block_solver
add_executable(block_solver main_block_solver.c)
target_link_libraries(block_solver PRIVATE bclib)

# block_template_parser
add_executable(block_template_parser main_block_template_parser.c)
target_link_libraries(block_template_parser PRIVATE bclib)

# block_dexor: bitcoin-cli provides xor-ed block data (files blk-xxx.dat)
add_executable(block_dexor main_block_dexor.c)
target_link_libraries(block_dexor PRIVATE bclib)

# block_builder
add_executable(block_builder main_block_builder.c)
target_link_libraries(block_builder PRIVATE bclib)

# client
add_executable(client tcp_client.c main_client.c)
target_link_libraries(client PRIVATE bclib)
