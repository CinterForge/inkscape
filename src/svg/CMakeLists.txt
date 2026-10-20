# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
	css-ostringstream.cpp
	path-string.cpp
    # sp-svg.def
	stringstream.cpp
	strip-trailing-zeros.cpp
	svg-affine.cpp
	svg-affine-parser.cpp
	svg-box.cpp
	svg-angle.cpp
	svg-length.cpp
	svg-bool.cpp
	svg-path.cpp

	# -------
	# Headers
	css-ostringstream.h
	path-string.h
	stringstream.h
	strip-trailing-zeros.h
	svg-box.h
	svg-angle.h
	svg-length.h
	svg-bool.h
	svg.h

)
