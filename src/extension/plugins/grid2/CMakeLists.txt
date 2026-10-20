# SPDX-License-Identifier: GPL-2.0-or-later

add_library(grid2 SHARED EXCLUDE_FROM_ALL grid.cpp)

target_link_libraries(grid2 inkscape_base)
target_include_directories(grid2 PRIVATE ${CMAKE_BINARY_DIR}/src)
