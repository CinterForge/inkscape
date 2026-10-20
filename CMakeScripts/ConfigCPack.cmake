############################
# CPack configuration file #
############################


## General ##

set(CPACK_PACKAGE_NAME "Inkscape")
set(CPACK_PACKAGE_VENDOR "Inkscape")
set(CPACK_PACKAGE_VERSION_MAJOR ${INKSCAPE_VERSION_MAJOR}) # TODO: Can be set via project(), see CMAKE_PROJECT_VERSION_PATCH
set(CPACK_PACKAGE_VERSION_MINOR ${INKSCAPE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${INKSCAPE_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}${INKSCAPE_VERSION_SUFFIX}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md") # TODO: Where is this used? Do we need a better source?
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source vector graphics editor")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://inkscape.org")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.svg") # TODO: Can any generator make use of this?
set(CPACK_PACKAGE_CONTACT "Inkscape developers <inkscape-devel@lists.inkscape.org>")

set(CPACK_PACKAGE_FILE_NAME ${INKSCAPE_DIST_PREFIX})
set(CPACK_PACKAGE_CHECKSUM "SHA256")

set(CPACK_PACKAGE_EXECUTABLES "inkscape;Inkscape;inkview;Inkview")
set(CPACK_CREATE_DESKTOP_LINKS "inkscape")

if(WIN32)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Inkscape")
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "inkscape")
    set(CPACK_STRIP_FILES TRUE)
endif()

# This creates a screen in the windows installers asking users to "Agree" to the GPL which is incorrect.
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
# set( CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/README.md") # TODO: can we use this?

set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)

# specific config for source packaging (note this is used by the 'dist' target)
set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${INKSCAPE_DIST_PREFIX})
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;/obj*/;cscope.*;.gitlab*;.coveragerc;*.md;")




# this allows to override above configuration per cpack generator at CPack-time
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake")
configure_file("${CMAKE_SOURCE_DIR}/CMakeScripts/CPack.cmake" "${CMAKE_BINARY_DIR}/CMakeScripts/CPack.cmake" @ONLY)




## Generator-specific configuration ##

# NSIS (Windows .exe installer)
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_SOURCE_DIR}/packaging/nsis/header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/packaging/nsis/welcomefinish.bmp")
set(CPACK_NSIS_IGNORE_LICENSE_PAGE 1)
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/inkscape.exe")
set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_MENU_LINKS "${CPACK_PACKAGE_HOMEPAGE_URL}" "Inkscape Homepage")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma") # zlib|bzip2|lzma
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
set(CPACK_NSIS_MODIFY_PATH "ON") # while the name does not suggest it, this also provides the possibility to add desktop icons
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "inkscape") # TODO: this results in instance with administrative privileges!
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON) # Make the text not blurry in installer

# Additional commands placed very early in the NSIS file.
# ( Unfortunately the CPack NSIS template contains not enough usable variables to place these commands.
# As a workaround, we put them into CPACK_NSIS_COMPRESSOR.
# https://github.com/Kitware/CMake/blob/8fe9454bcca64a327fb3e0ccf5aff7477eb036cc/Modules/Internal/CPack/NSIS.template.in )
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  SetCompressorDictSize 64") # hack (improve compression)
# overwrite BrandingText
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  BrandingText '${CPACK_PACKAGE_DESCRIPTION_SUMMARY}'")
# better components page layout
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  !define MUI_COMPONENTSPAGE_SMALLDESC")
# remove previous version(s) before installation starts
# (could be placed later in the NSIS file but there is no appropriate variable)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/nsis/uninstall-old-versions.nsh" native_path)
string(REPLACE "\\" "\\\\" native_path "${native_path}")
set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  !include '${native_path}'")

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/nsis/fileassoc.nsh" native_path)
string(REPLACE "\\" "\\\\" native_path "${native_path}")
set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "!include '${native_path}'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\
  WriteRegStr SHCTX 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\inkscape.exe' '' '$INSTDIR\\\\bin\\\\inkscape.exe'\n\
  WriteRegStr SHCTX 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\inkscape.exe' 'Path' '$INSTDIR\\\\bin'\n\
  !insertmacro APP_ASSOCIATE 'svg' 'Inkscape.SVG' 'Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  !insertmacro APP_ASSOCIATE 'svgz' 'Inkscape.SVGZ' 'Compressed Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  !insertmacro UPDATEFILEASSOC")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\
  DeleteRegKey SHCTX 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\inkscape.exe'\n\
  !insertmacro APP_UNASSOCIATE 'svg' 'Inkscape.SVG'\n\
  !insertmacro APP_UNASSOCIATE 'svgz' 'Inkscape.SVGZ'\n\
  !insertmacro UPDATEFILEASSOC")

# WIX (Windows .msi installer)
set(CPACK_WIX_UPGRADE_GUID "4d5fedaa-84a0-48be-bd2a-08246398361a")
set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/banner.bmp")
set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/packaging/wix/Bitmaps/dialog.bmp")
set(CPACK_WIX_UI_REF "WixUI_FeatureTree_nolicense")
set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_WIX_PROPERTY_ARPURLUPDATEINFO "${CPACK_PACKAGE_HOMEPAGE_URL}/release")
set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")

set(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/packaging/wix/app_registration.xml"
                         "${CMAKE_SOURCE_DIR}/packaging/wix/feature_attributes.xml")
set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/packaging/wix/WixUI_FeatureTree_nolicense.wxs")
set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/packaging/wix/WIX.template.in")

# DEB (Linux .deb bundle)
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_INKSCAPE_PACKAGE_SUGGESTS "dia, pstoedit, scribus")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "inkscape") # all package depend on inkscape
set(CPACK_DEBIAN_INKSCAPE_PACKAGE_DEPENDS "lib2geom${2GEOM_VERSION}")
set(CPACK_DEBIAN_INKSCAPE_PACKAGE_RECOMMENDS "aspell, imagemagick, libwmf-bin, inkscape-extensions, inkscape-translations, inkscape-themes")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING IGNORE)

set(CPACK_DEBIAN_INKSCAPE-EXTENSIONS_PACKAGE_DEPENDS "python3-numpy, python3-lxml, python3-scour, python3-packaging, python3-cssselect, python3-webencodings, python3-tinycss2, python3-bs4, python3-requests")
set(CPACK_DEBIAN_INKSCAPE-EXTENSIONS_PACKAGE_RECOMMENDS "inkscape-extension-manager")
set(CPACK_DEBIAN_INKSCAPE-EXTENSION-MANAGER_PACKAGE_DEPENDS "inkscape-extensions, python3-filecache")
set(CPACK_DEBIAN_LIB2GEOM-DEV_PACKAGE_RECOMMENDS "lib2geom${2GEOM_VERSION}")
set(CPACK_DEBIAN_LIB2GEOM${2GEOM_VERSION}_PACKAGE_NAME "lib2geom${2GEOM_VERSION}")
set(CPACK_DEBIAN_LIB2GEOM-DEV_PACKAGE_NAME "lib2geom-dev")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_ADD_LDCONFIG_CALL ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

## load cpack module (do this *after* all the CPACK_* variables have been set)
include(CPack)




## Component definition
#  - variable names are UPPER CASE, even if component names are lower case
#  - components/groups are ordered alphabetically by component/group name; groups always come first
#  - empty (e.g. OS-specific) components are discarded automatically

cpack_add_install_type(full    DISPLAY_NAME Full)
cpack_add_install_type(compact DISPLAY_NAME Compact) # no translations, dictionaries, examples, etc.
cpack_add_install_type(minimal DISPLAY_NAME Minimal) # minimal "working" set-up, excluding everything non-essential

cpack_add_component_group(
                    group_1_program_files
                    DISPLAY_NAME "Program Files"
                    EXPANDED)
cpack_add_component(inkscape
                    DISPLAY_NAME "Inkscape SVG Editor"
                    DESCRIPTION "Inkscape core files and dependencies"
                    GROUP "group_1_program_files"
                    REQUIRED)
cpack_add_component(python
                    DISPLAY_NAME "Python"
                    DESCRIPTION "Python interpreter (required to run Inkscape extensions)"
                    GROUP "group_1_program_files"
                    INSTALL_TYPES full compact)

cpack_add_component_group(
                    group_2_inkscape_data
                    DISPLAY_NAME "Inkscape Data"
                    EXPANDED)
cpack_add_component(extensions
                    DISPLAY_NAME "Extensions"
                    DESCRIPTION "Inkscape extensions (including many import and export plugins)"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full compact)
cpack_add_component(extension_manager
                    DISPLAY_NAME "Extension Manager"
                    DESCRIPTION "Extension manager allows user to install extensions from the inkscape website"
                    DEPENDS "extensions"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full compact)
cpack_add_component(themes
                    DISPLAY_NAME "Themes"
                    DESCRIPTION "Inkscape themes (look and feel including icons)"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full compact)
cpack_add_component(examples
                    DISPLAY_NAME "Examples"
                    DESCRIPTION "Example files created in Inkscape"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full)
cpack_add_component(tutorials
                    DISPLAY_NAME "Tutorials"
                    DESCRIPTION "Tutorials teaching Inkscape usage"
                    GROUP "group_2_inkscape_data"
                    INSTALL_TYPES full)

cpack_add_component(dictionaries
                    DISPLAY_NAME "Dictionaries"
                    DESCRIPTION "Dictionaries for some common languages for spell checking in Inkscape"
                    INSTALL_TYPES full)

cpack_add_component_group(
                    group_3_translations
                    DISPLAY_NAME "Translations"
                    DESCRIPTION "Translations and localized content for Inkscape")
get_inkscape_languages()
list(LENGTH INKSCAPE_LANGUAGE_CODES length)
math(EXPR length "${length} - 1")
if(WIN32)
    foreach(index RANGE ${length})
        list(GET INKSCAPE_LANGUAGE_CODES ${index} language_code)
        list(GET INKSCAPE_LANGUAGE_NAMES ${index} language_name)
        string(MAKE_C_IDENTIFIER "${language_code}" language_code_escaped)
        cpack_add_component(translations.${language_code_escaped}
                            DISPLAY_NAME "${language_name}"
                            GROUP "group_3_translations"
                            INSTALL_TYPES full)
    endforeach()
else()
    cpack_add_component(translations
                        DISPLAY_NAME "Translations"
                        GROUP "group_3_translations"
                        INSTALL_TYPES full)
endif()

if(WITH_INTERNAL_2GEOM)
    cpack_add_component(lib2geom_dev
                        DISPLAY_NAME "lib2geom-dev"
                        DESCRIPTION "Geometry library - dev files")
    cpack_add_component(lib2geom${2GEOM_VERSION}
                        DISPLAY_NAME "lib2geom"
                        DESCRIPTION "Geometry library"
                        HIDDEN REQUIRED)
endif()
