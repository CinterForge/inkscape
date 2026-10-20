# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
	console-output-undo-observer.cpp
	demangle.cpp
	heap.cpp
	logger.cpp
	sysv-heap.cpp
	timestamp.cpp

	# ------
	# Header
	console-output-undo-observer.h
	demangle.h
	event-tracker.h
	event.h
	gc-heap.h
	heap.h
	logger.h
	simple-event.h
	sysv-heap.h
	timestamp.h
)
