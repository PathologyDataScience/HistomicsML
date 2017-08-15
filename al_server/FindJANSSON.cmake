# - Find JANSSON (includes and library)
#
# This module defines
#  JANSSON_INCLUDE_DIR
#  JANSSON_LIBRARIES
#  JANSSON_FOUND
#
# Also defined, but not for general use are
#  CBLAS_LIBRARY, where to find the library.

set(JANSSON_FIND_REQUIRED true)

find_path(JANSSON_INCLUDE_DIR jansson.h
    /usr/include/
    /usr/local/include/
)

set(JANSSON_NAMES ${JANSSON_NAMES} jansson)


find_library(JANSSON_LIBRARY
    NAMES ${JANSSON_NAMES}
    PATHS
    /usr/lib64/
    /usr/lib/
    /usr/local/lib64/
    /usr/local/lib/
)



if (JANSSON_LIBRARY AND JANSSON_INCLUDE_DIR)
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
    set(JANSSON_FOUND true)
endif (JANSSON_LIBRARY AND JANSSON_INCLUDE_DIR)



# Hide in the cmake cache
mark_as_advanced(JANSSON_LIBRARIESS CBLAS_INCLUDE_DIR)