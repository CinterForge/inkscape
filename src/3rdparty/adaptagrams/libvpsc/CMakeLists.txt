add_library(vpsc_LIB STATIC)

target_sources(vpsc_LIB PRIVATE
	block.cpp
	blocks.cpp
	cbuffer.cpp
	constraint.cpp
	rectangle.cpp
	solve_VPSC.cpp
	variable.cpp

	assertions.h
	block.h
	blocks.h
	cbuffer.h
	constraint.h
	exceptions.h
	linesegment.h
	pairing_heap.h
	rectangle.h
	solve_VPSC.h
	variable.h
)

target_include_directories(vpsc_LIB PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
