add_executable(simple simple.cpp)
target_link_libraries(simple PRIVATE liblmdb)

add_executable(simple_cpp simple_cpp.cpp)
target_link_libraries(simple_cpp PRIVATE liblmdbpp)
