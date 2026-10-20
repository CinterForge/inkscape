# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.xml" "*.rc" "*.css" "*.ui" "*.glade" "*.svg" "*.ini" "*.lang")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/ui)

file(GLOB _RESOURCES "resources/*.png" "resources/*.svg" "resources/*.txt")
install(FILES ${_RESOURCES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/ui/resources)

file(GLOB _SYNTAX_THEMES "syntax-themes/*.xml")
install(FILES ${_SYNTAX_THEMES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/ui/syntax-themes)
