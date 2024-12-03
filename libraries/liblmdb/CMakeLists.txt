project(liblmdb)

set(SRCS
    ${CMAKE_CURRENT_LIST_DIR}/mdb.c
    ${CMAKE_CURRENT_LIST_DIR}/mdb_copy.c
    ${CMAKE_CURRENT_LIST_DIR}/mdb_drop.c
    ${CMAKE_CURRENT_LIST_DIR}/mdb_dump.c
    ${CMAKE_CURRENT_LIST_DIR}/mdb_load.c
    ${CMAKE_CURRENT_LIST_DIR}/mdb_stat.c
    ${CMAKE_CURRENT_LIST_DIR}/midl.c
    ${CMAKE_CURRENT_LIST_DIR}/mplay.c)

add_library(liblmdb STATIC ${SRCS})
