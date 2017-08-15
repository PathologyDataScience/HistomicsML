# - Find LIBCONFIG (includes and library)
#
# This module defines
#  LIBCONFIG_INCLUDE_DIR
#  LIBCONFIG_LIBRARIES
#  LIBCONFIG_FOUND
#

set(LIBCONFIG_FIND_REQUIRED true)

find_path(LIBCONFIG_INCLUDE_DIR libconfig.h++
    /usr/include/
    /usr/local/include/
    /opt/include
)

set(LIBCONFIG_NAMES ${LIBCONFIG_NAMES} config++)


find_library(LIBCONFIG_LIBRARY
    NAMES ${LIBCONFIG_NAMES}
    PATHS
    /usr/lib64/
    /usr/lib/
    /usr/local/lib64/
    /usr/local/lib/
    /opt/lib/
)


if (LIBCONFIG_LIBRARY AND LIBCONFIG_INCLUDE_DIR)
    set(LIBCONFIG_LIBRARIES ${LIBCONFIG_LIBRARY})
    set(LIBCONFIG_FOUND true)
endif (LIBCONFIG_LIBRARY AND LIBCONFIG_INCLUDE_DIR)



# Hide in the cmake cache
mark_as_advanced(LIBCONFIG_LIBRARIES LIBCONFIG_INCLUDE_DIR)