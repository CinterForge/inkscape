# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.ttf")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/fonts)
