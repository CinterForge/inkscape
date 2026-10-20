# SPDX-License-Identifier: GPL-2.0-or-later
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/markers.svg.h
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${CMAKE_CURRENT_SOURCE_DIR}/markers.svg > ${CMAKE_CURRENT_BINARY_DIR}/markers.svg.h
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/markers.svg
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py
)
set_source_files_properties(${CMAKE_BINARY_DIR}/markers.svg.h PROPERTIES GENERATED TRUE)

add_custom_target(markers_svg_h ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/markers.svg.h)

file(GLOB _FILES "*.svg")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/markers)
