# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.svg" "*.svgz" "*.pov")
if(WIN32)
install(FILES ${_FILES}
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/examples
    COMPONENT examples)
else()
install(FILES ${_FILES}
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/examples
    COMPONENT inkscape)
endif()
