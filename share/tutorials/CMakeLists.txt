# SPDX-License-Identifier: GPL-2.0-or-later

file(GLOB _FILES "*.svg" "*.jpg" "*.png")

filter_and_install_translated_content(_FILES ${INKSCAPE_SHARE_INSTALL}/tutorials)
if(WIN32)
install(FILES ${_FILES}
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/tutorials
    COMPONENT tutorials)
else()
install(FILES ${_FILES}
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/tutorials
    COMPONENT inkscape)
endif()
