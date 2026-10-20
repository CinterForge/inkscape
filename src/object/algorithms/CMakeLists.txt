# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
  graphlayout.cpp
  removeoverlap.cpp
  unclump.cpp

  # -------
  # Headers
  bboxsort.h
  graphlayout.h
  removeoverlap.h
  unclump.h
)
