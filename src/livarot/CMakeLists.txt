# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
	AVL.cpp
	float-line.cpp
	PathConversion.cpp
	Path.cpp
	PathCutting.cpp
	path-description.cpp
	PathOutline.cpp
	PathSimplify.cpp
	PathStroke.cpp
	Shape.cpp
	ShapeDraw.cpp
	ShapeMisc.cpp
	ShapeRaster.cpp
	ShapeSweep.cpp
	sweep-event.cpp
	sweep-tree.cpp
	sweep-tree-list.cpp

	# Headers
	AVL.h
	LivarotDefs.h
	Path.h
	Shape.h
	float-line.h
	path-description.h
	sweep-event-queue.h
	sweep-event.h
	sweep-tree-list.h
	sweep-tree.h
)
