add_library(autotrace_LIB STATIC)

set(INC include/autotrace)
set(SRC src/autotrace)
target_sources(autotrace_LIB PRIVATE
    ${SRC}/config.h
    ${SRC}/autotrace.c
    ${INC}/autotrace.h
    ${SRC}/bitmap.c
    ${SRC}/bitmap.h
    ${SRC}/color.c
    ${INC}/color.h
    ${SRC}/curve.c
    ${SRC}/curve.h
    ${SRC}/despeckle.c
    ${SRC}/despeckle.h
    ${SRC}/epsilon-equal.c
    ${SRC}/epsilon-equal.h
    ${SRC}/exception.c
    ${INC}/exception.h
    ${SRC}/filename.c
    ${SRC}/filename.h
    ${SRC}/fit.c
    ${SRC}/fit.h
    ${SRC}/image-header.h
    ${SRC}/image-proc.c
    ${SRC}/image-proc.h
    ${SRC}/input.c
    ${INC}/input.h
    ${SRC}/intl.h
    ${SRC}/logreport.c
    ${SRC}/logreport.h
    ${SRC}/median.c
    ${SRC}/module.c
    ${SRC}/output.c
    ${INC}/output.h
    ${SRC}/private.h
    ${SRC}/pxl-outline.c
    ${SRC}/pxl-outline.h
    ${SRC}/quantize.h
    ${SRC}/spline.c
    ${INC}/spline.h
    ${SRC}/thin-image.c
    ${SRC}/thin-image.h
    ${INC}/types.h
    ${SRC}/vector.c
    ${SRC}/vector.h
    ${SRC}/xstd.h
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
target_link_libraries(autotrace_LIB PUBLIC PkgConfig::GLIB Intl::Intl)
target_include_directories(autotrace_LIB PUBLIC SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(autotrace_LIB PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/autotrace)
