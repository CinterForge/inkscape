# SPDX-License-Identifier: GPL-2.0-or-later

target_sources(inkscape_base PRIVATE
  sp-filter-primitive.cpp
  blend.cpp
  colormatrix.cpp
  componenttransfer-funcnode.cpp
  componenttransfer.cpp
  composite.cpp
  convolvematrix.cpp
  diffuselighting.cpp
  displacementmap.cpp
  distantlight.cpp
  dropshadow.cpp
  flood.cpp
  gaussian-blur.cpp
  image.cpp
  merge.cpp
  mergenode.cpp
  morphology.cpp
  offset.cpp
  pointlight.cpp
  specularlighting.cpp
  spotlight.cpp
  slot-resolver.cpp
  tile.cpp
  turbulence.cpp

  # -------
  # Headers
  sp-filter-primitive.h
  blend.h
  colormatrix.h
  componenttransfer-funcnode.h
  componenttransfer.h
  composite.h
  convolvematrix.h
  diffuselighting.h
  displacementmap.h
  distantlight.h
  dropshadow.h
  flood.h
  gaussian-blur.h
  image.h
  merge.h
  mergenode.h
  morphology.h
  offset.h
  pointlight.h
  specularlighting.h
  spotlight.h
  slot-resolver.h
  tile.h
  turbulence.h
)
