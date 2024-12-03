project(liblmdbpp)

set(SRCS ${CMAKE_CURRENT_LIST_DIR}/lmdb.cpp)

add_library(liblmdbpp STATIC ${SRCS})
target_link_libraries(liblmdbpp PUBLIC liblmdb)
target_include_directories(liblmdb PUBLIC ${CMAKE_SOURCE_DIR}/cxx)
