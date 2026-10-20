include(${CMAKE_CURRENT_LIST_DIR}/ConfigPaths.cmake)

# Set all HAVE_XXX variables, to correctly set all defines in config.h
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckCXXSymbolExists)

CHECK_INCLUDE_FILES(ieeefp.h HAVE_IEEEFP_H)
CHECK_FUNCTION_EXISTS(mallinfo HAVE_MALLINFO)
CHECK_FUNCTION_EXISTS(mallinfo2 HAVE_MALLINFO2)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_STRUCT_HAS_MEMBER("struct mallinfo" fordblks malloc.h HAVE_STRUCT_MALLINFO_FORDBLKS )
CHECK_STRUCT_HAS_MEMBER("struct mallinfo" fsmblks  malloc.h HAVE_STRUCT_MALLINFO_FSMBLKS  )
CHECK_STRUCT_HAS_MEMBER("struct mallinfo" hblkhd   malloc.h HAVE_STRUCT_MALLINFO_HBLKHD   )
CHECK_STRUCT_HAS_MEMBER("struct mallinfo" uordblks malloc.h HAVE_STRUCT_MALLINFO_UORDBLKS )
CHECK_STRUCT_HAS_MEMBER("struct mallinfo" usmblks  malloc.h HAVE_STRUCT_MALLINFO_USMBLKS  )
CHECK_CXX_SYMBOL_EXISTS(sincos math.h HAVE_SINCOS)  # 2geom define

# Create the configuration files config.h in the binary root dir
configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
add_definitions(-DHAVE_CONFIG_H)
