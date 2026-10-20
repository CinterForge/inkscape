add_library(cola_LIB STATIC)

target_sources(cola_LIB PRIVATE
	box.cpp
    cluster.cpp
	cola.cpp
	convex_hull.cpp
	compound_constraints.cpp
	conjugate_gradient.cpp
	connected_components.cpp
	gradient_projection.cpp
	shapepair.cpp
	straightener.cpp

	box.h
    cluster.h
	cola.h
	compound_constraints.h
	conjugate_gradient.h
	convex_hull.h
	gradient_projection.h
	shapepair.cpp
	shortest_paths.h
	straightener.h
)

target_include_directories(cola_LIB PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
