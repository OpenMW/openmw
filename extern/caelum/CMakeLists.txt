project(Caelum)

ADD_DEFINITIONS(-DCAELUM_LIB)
INCLUDE_DIRECTORIES( ${CMAKE_HOME_DIRECTORY}/extern/caelum/include )

file(GLOB_RECURSE CAELUM_SRC src/*)
file(GLOB_RECURSE CAELUM_HDR include/*)

set(SOURCES ${CAELUM_SRC} ${CAELUM_HDR})
add_library(caelum STATIC ${SOURCES})

