# SPDX-License-Identifier: GPL-2.0-or-later

set(libnrtype_SRC
        font-lister.cpp
        font-utils.cpp
	Layout-TNG.cpp
	Layout-TNG-Compute.cpp
	Layout-TNG-Input.cpp
	Layout-TNG-OutIter.cpp
	Layout-TNG-Output.cpp
	Layout-TNG-Scanline-Makers.cpp
	style-attachments.cpp

	# -------
	# Headers
	font-lister.h
        font-utils.h
	Layout-TNG-Scanline-Maker.h
	Layout-TNG.h
	style-attachments.h
)

include("${CMAKE_CURRENT_LIST_DIR}/CMakeUnit.txt")

target_sources(inkscape_base PRIVATE ${libnrtype_unit_SRC} ${libnrtype_SRC})
