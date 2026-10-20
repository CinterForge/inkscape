# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
	cielab.cpp
	filterset.cpp
	imagemap.cpp
	imagemap-gdk.cpp
	quantize.cpp
	siox.cpp
	trace.cpp

	potrace/inkscape-potrace.cpp
	autotrace/inkscape-autotrace.cpp
	depixelize/inkscape-depixelize.cpp

	# -------
	# Headers
	cielab.h
	filterset.h
	imagemap-gdk.h
	imagemap.h
	pool.h
	quantize.h
	siox.h
	trace.h

	potrace/bitmap.h
	potrace/inkscape-potrace.h
	autotrace/inkscape-autotrace.h
	depixelize/inkscape-depixelize.h
)
