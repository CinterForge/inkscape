add_subdirectory(libavoid)
add_subdirectory(libcola)
add_subdirectory(libvpsc)

add_library(adaptagrams_LIB INTERFACE)
target_link_libraries(adaptagrams_LIB INTERFACE avoid_LIB cola_LIB vpsc_LIB)
