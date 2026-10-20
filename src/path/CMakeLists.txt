# SPDX-License-Identifier: GPL-2.0-or-later

# Non-unit testable files
set(path_SRC
    splinefit/bezier-fit.cpp
    splinefit/splinefit.c
    splinefit/splinefont.c
    splinefit/splinerefigure.c
    path-object-set.cpp   # Needs preferences
    path-offset.cpp       # Needs preferences
    path-simplify.cpp
    path-outline.cpp
    path-util.cpp

    splinefit/bezier-fit.h
    splinefit/splinefit.h
    splinefit/splinefont.h
    splinefit/splinerefigure.h
    path-offset.h
    path-simplify.h
    path-outline.h
    path-util.h
)

include("${CMAKE_CURRENT_LIST_DIR}/CMakeUnit.txt")

target_sources(inkscape_base PRIVATE ${path_unit_SRC} ${path_SRC})
