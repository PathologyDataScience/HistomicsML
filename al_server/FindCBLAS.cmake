# - Find CBLAS (includes and library)
#
# This module defines
#  CBLAS_INCLUDE_DIR
#  CBLAS_LIBRARIES
#  CBLAS_FOUND
#
# Also defined, but not for general use are
#  CBLAS_LIBRARY, where to find the library.

set(BLAS_FIND_REQUIRED true)

find_path(CBLAS_INCLUDE_DIR cblas.h
    /usr/include/atlas
    /usr/local/include/atlas
    /usr/include
    /usr/local/include
    /System/Library/Frameworks/Accelerate.framework/Versions/Current/Frameworks/vecLib.framework/Headers    
)

set(CBLAS_NAMES ${CBLAS_NAMES} cblas)


find_library(CBLAS_LIBRARY
    NAMES ${CBLAS_NAMES}
    PATHS
    /usr/lib64/atlas
    /usr/lib/atlas
	/usr/lib/atlas-base
    /usr/local/lib64/atlas
    /usr/local/lib/atlas
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
)

if (CBLAS_LIBRARY AND CBLAS_INCLUDE_DIR)
    set(CBLAS_LIBRARIES ${CBLAS_LIBRARY})
    set(CBLAS_FOUND true)
endif (CBLAS_LIBRARY AND CBLAS_INCLUDE_DIR)



# Deprecated declarations.
set(NATIVE_CBLAS_INCLUDE_PATH ${CBLAS_INCLUDE_DIR} )
GET_FILENAME_COMPONENT(NATIVE_CBLAS_LIB_PATH ${CBLAS_LIBRARY} PATH)


# Hide in the cmake cache
mark_as_advanced(CBLAS_LIBRARY CBLAS_INCLUDE_DIR)
