#  POTRACE_FOUND - system has Potrace
#  POTRACE_INCLUDE_DIRS - the Potrace include directory
#  POTRACE_LIBRARIES - The libraries needed to use Potrace
#  Potrace::Potrace - imported target

if(POTRACE_LIBRARIES AND POTRACE_INCLUDE_DIRS)
   # in cache already
   set(POTRACE_FOUND TRUE)
else()
  find_path(POTRACE_INCLUDE_DIR
    NAMES
      potracelib.h
    PATHS
      /usr/include
      /usr/local/include
    PATH_SUFFIXES
      potrace
  )

  find_library(POTRACE_LIBRARY
    NAMES
      potrace
      libpotrace
    PATHS
      /usr/lib
      /usr/local/lib
  )

  if(POTRACE_LIBRARY)
    set(POTRACE_FOUND TRUE)
  endif()

  set(POTRACE_INCLUDE_DIRS
    ${POTRACE_INCLUDE_DIR}
  )

  if (POTRACE_FOUND)
    set(POTRACE_LIBRARIES
      ${POTRACE_LIBRARIES}
      ${POTRACE_LIBRARY}
    )
  endif()

  if(POTRACE_INCLUDE_DIRS AND POTRACE_LIBRARIES)
    set(POTRACE_FOUND TRUE)
  endif()
  
  if(POTRACE_FOUND)
    add_library(Potrace::Potrace INTERFACE IMPORTED)
    target_include_directories(Potrace::Potrace INTERFACE ${POTRACE_INCLUDE_DIR})
    target_link_libraries(Potrace::Potrace INTERFACE ${POTRACE_LIBRARIES})
  endif()

  if(POTRACE_FOUND)
    if(NOT Potrace_FIND_QUIETLY)
      message(STATUS "Found Potrace: ${POTRACE_LIBRARIES}")
    endif()
  else()
    if(Potrace_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find potrace")
    endif()
  endif()

  # show the POTRACE_INCLUDE_DIRS and POTRACE_LIBRARIES variables only in the advanced view
  mark_as_advanced(POTRACE_INCLUDE_DIRS POTRACE_LIBRARIES)
endif()
