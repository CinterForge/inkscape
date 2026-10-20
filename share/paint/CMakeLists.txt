# SPDX-License-Identifier: GPL-2.0-or-later

# translate labels from SVG files
file(GLOB _FILES "*.svg")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/patterns.svg.h
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES} > ${CMAKE_CURRENT_BINARY_DIR}/patterns.svg.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES}
)
set_source_files_properties(${CMAKE_BINARY_DIR}/patterns.svg.h PROPERTIES GENERATED TRUE)
add_custom_target(patterns_svg_h ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/patterns.svg.h)

install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/paint)
