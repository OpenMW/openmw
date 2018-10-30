find_path(RecastNavigation_INCLUDE_DIR
    NAMES Recast.h
    HINTS $ENV{RecastNavigation_ROOT}
        ${RecastNavigation_ROOT}
    PATH_SUFFIXES include
)
mark_as_advanced(RecastNavigation_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

set(RecastNavigation_LIBRARIES "")

foreach(COMPONENT ${RecastNavigation_FIND_COMPONENTS})
    if(NOT RecastNavigation_${COMPONENT}_FOUND)
        find_library(RecastNavigation_${COMPONENT}_LIBRARY
            HINTS $ENV{RecastNavigation_ROOT}
                ${RecastNavigation_ROOT}
            NAMES ${COMPONENT}
            PATH_SUFFIXES lib
        )
        find_package_handle_standard_args(RecastNavigation_${COMPONENT} DEFAULT_MSG
            RecastNavigation_${COMPONENT}_LIBRARY
            RecastNavigation_INCLUDE_DIR
        )
        mark_as_advanced(RecastNavigation_${COMPONENT}_LIBRARY)
        if(RecastNavigation_${COMPONENT}_FOUND)
            list(APPEND RecastNavigation_LIBRARIES ${RecastNavigation_${COMPONENT}_LIBRARY})
        endif()
    endif()
endforeach()
mark_as_advanced(RecastNavigation_LIBRARIES)

find_package_handle_standard_args(RecastNavigation DEFAULT_MSG
    RecastNavigation_LIBRARIES
    RecastNavigation_INCLUDE_DIR
)

if(RecastNavigation_FOUND)
    set(RecastNavigation_INCLUDE_DIRS ${RecastNavigation_INCLUDE_DIR})
endif()
