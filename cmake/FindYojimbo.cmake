SET(Yojimbo_INCLUDES
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/include
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/netcode
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/reliable
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/serialize
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/sodium
  ${CMAKE_SOURCE_DIR}/extern/yojimbo/tlsf
)

include_directories(${Yojimbo_INCLUDES})

SET(Yojimbo_Root ${CMAKE_SOURCE_DIR}/extern/yojimbo)
SET(Yojimbo_Output ${Yojimbo_Root}/bin)

MACRO(add_yojimbo_library NAME)
  find_library(${NAME}_LIBRARY
    NAMES lib${NAME}
    HINTS ${Yojimbo_Output}
  )

  IF(NOT ${${NAME}_LIBRARY})
    file(GLOB ${NAME}_Sources
      "${Yojimbo_Root}/${NAME}/${NAME}.h"
      "${Yojimbo_Root}/${NAME}/${NAME}.c"
    )

    add_library(${NAME} STATIC ${${NAME}_Sources})

    set_target_properties(${NAME} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${Yojimbo_Output}
    )

    IF(WIN32)
      set(${${NAME}_LIBRARY} ${Yojimbo_Output}/lib${NAME}.lib)
    ELSE()
      set(${${NAME}_LIBRARY} ${Yojimbo_Output}/lib${NAME}.a)
    ENDIF()
  ENDIF()
  set(Yojimbo_LIBRARIES ${Yojimbo_LIBRARIES} ${NAME})
ENDMACRO()

add_yojimbo_library(tlsf)
add_yojimbo_library(netcode)
add_yojimbo_library(reliable)

find_library(Yojimbo_LIBRARY
  NAMES libyojimbo
  HINTS ${Yojimbo_Output}
)

find_library(Sodium_LIBRARY
  NAMES libsodium
  HINTS ${Yojimbo_Output}
)

IF(NOT Yojimbo_LIBRARY)
  file(GLOB Yojimbo_Sources
    "${Yojimbo_Root}/include/*.h"
    "${Yojimbo_Root}/source/*.cpp"
  )

  add_library(yojimbo STATIC ${Yojimbo_Sources})

  set_target_properties(yojimbo PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${Yojimbo_Output}
  )

  if (WIN32)
    set(Yojimbo_LIBRARY ${Yojimbo_Output}/libyojimbo.lib)
  else ()
    set(Yojimbo_LIBRARY ${Yojimbo_Output}/libyojimbo.a)
  endif ()

  set(Yojimbo_LIBRARIES ${Yojimbo_LIBRARIES} yojimbo)

ENDIF(NOT Yojimbo_LIBRARY)

IF(NOT Sodium_LIBRARY)
  file(GLOB Sodium_Sources
    IF (WIN32)
      "${Yojimbo_Root}/sodium/*.c"
      "${Yojimbo_Root}/sodium/*.h"
    ELSE()
      "${Yojimbo_Root}/sodium/*.S"
    ENDIF()
  )

  add_library(sodium STATIC ${Sodium_Sources})

  set_target_properties(sodium PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${Yojimbo_Output}
  )

  if (WIN32)
    set(Sodium_LIBRARY ${Yojimbo_Output}/libsodium.lib)
  else ()
    set(Sodium_LIBRARY ${Yojimbo_Output}/libsodium.a)
  endif ()

  set(Yojimbo_LIBRARIES ${Yojimbo_LIBRARIES} sodium)

ENDIF(NOT Sodium_LIBRARY)
