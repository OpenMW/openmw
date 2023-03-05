
# Locate Collada
# This module defines:
# COLLADA_INCLUDE_DIR, where to find the headers
#
# COLLADA_LIBRARY, COLLADA_LIBRARY_DEBUG
# COLLADA_FOUND, if false, do not try to link to Collada dynamically
#
# COLLADA_LIBRARY_STATIC, COLLADA_LIBRARY_STATIC_DEBUG
# COLLADA_STATIC_FOUND, if false, do not try to link to Collada statically
#
# $COLLADA_DIR is an environment variable that would
# correspond to the ./configure --prefix=$COLLADA_DIR
#
# Created by Robert Osfield.


# Check if COLLADA_DIR is set, otherwise use ACTUAL_3DPARTY_DIR:
SET( COLLADA_ENV_VAR_AVAILABLE $ENV{COLLADA_DIR} )
IF ( COLLADA_ENV_VAR_AVAILABLE )
    SET(COLLADA_DOM_ROOT "$ENV{COLLADA_DIR}/dom" CACHE PATH "Location of Collada DOM directory" FORCE)
ELSE ()
    SET(COLLADA_DOM_ROOT "${ACTUAL_3DPARTY_DIR}/include/1.4/dom" CACHE PATH "Location of Collada DOM directory" FORCE)
ENDIF()


IF(APPLE)
    SET(COLLADA_BUILDNAME "mac")
    SET(COLLADA_BOOST_BUILDNAME ${COLLADA_BUILDNAME})
ELSEIF(MINGW)
    SET(COLLADA_BUILDNAME "mingw")
    SET(COLLADA_BOOST_BUILDNAME ${COLLADA_BUILDNAME})
ELSEIF((MSVC_VERSION GREATER 1910) OR (MSVC_VERSION EQUAL 1910))
    SET(COLLADA_BUILDNAME "vc14")
    SET(COLLADA_BOOST_BUILDNAME "vc141")
ELSEIF(MSVC_VERSION EQUAL 1900)
    SET(COLLADA_BUILDNAME "vc14")
    SET(COLLADA_BOOST_BUILDNAME "vc140")
ELSEIF(MSVC_VERSION EQUAL 1800)
    SET(COLLADA_BUILDNAME "vc12")
    SET(COLLADA_BOOST_BUILDNAME "vc120")
ELSEIF(MSVC_VERSION EQUAL 1700)
    SET(COLLADA_BUILDNAME "vc11")
    SET(COLLADA_BOOST_BUILDNAME "vc110")
ELSEIF(MSVC_VERSION EQUAL 1600)
    SET(COLLADA_BUILDNAME "vc10")
    SET(COLLADA_BOOST_BUILDNAME "vc100")
ELSEIF(MSVC_VERSION EQUAL 1500)
    SET(COLLADA_BUILDNAME "vc9")
    SET(COLLADA_BOOST_BUILDNAME "vc90")
ELSEIF(MSVC_VERSION EQUAL 1400)
    SET(COLLADA_BUILDNAME "vc8")
    SET(COLLADA_BOOST_BUILDNAME "vc80")
ELSE()
  SET(COLLADA_BUILDNAME "linux")
  SET(COLLADA_BOOST_BUILDNAME ${COLLADA_BUILDNAME})
ENDIF()

IF(${CMAKE_VS_PLATFORM_TOOLSET})
    string(REPLACE "v" "vc" COLLADA_BOOST_BUILDNAME ${CMAKE_VS_PLATFORM_TOOLSET})
ENDIF()


FIND_PATH(COLLADA_INCLUDE_DIR dae.h
    PATHS
    ${COLLADA_DOM_ROOT}/include
    $ENV{COLLADA_DIR}/include
    $ENV{COLLADA_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    /usr/local/include
    /usr/include/
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
    ${ACTUAL_3DPARTY_DIR}/include
    PATH_SUFFIXES
    colladadom
    collada-dom
    collada-dom2.5
    collada-dom2.4
    collada-dom2.2
)

FIND_LIBRARY(COLLADA_DYNAMIC_LIBRARY
    NAMES collada_dom collada14dom Collada14Dom libcollada14dom21 libcollada14dom22 collada-dom2.5-dp collada-dom2.5-dp-${COLLADA_BOOST_BUILDNAME}-mt collada-dom2.4-dp collada-dom2.4-dp-${COLLADA_BOOST_BUILDNAME}-mt
    PATHS
    ${COLLADA_DOM_ROOT}/build/${COLLADA_BUILDNAME}-1.4
    ${COLLADA_DOM_ROOT}
    $ENV{COLLADA_DIR}/build/${COLLADA_BUILDNAME}-1.4
    $ENV{COLLADA_DIR}/lib
    $ENV{COLLADA_DIR}/lib-dbg
    $ENV{COLLADA_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
    ${ACTUAL_3DPARTY_DIR}/lib
)

FIND_LIBRARY(COLLADA_DYNAMIC_LIBRARY_DEBUG
    NAMES collada_dom-d collada14dom-d Collada14Dom-d libcollada14dom21-d libcollada14dom22-d  collada-dom2.5-dp-d collada-dom2.5-dp-${COLLADA_BOOST_BUILDNAME}-mt-d collada-dom2.4-dp-d collada-dom2.4-dp-${COLLADA_BOOST_BUILDNAME}-mt-d
    PATHS
    ${COLLADA_DOM_ROOT}/build/${COLLADA_BUILDNAME}-1.4-d
    ${COLLADA_DOM_ROOT}
    $ENV{COLLADA_DIR}/build/${COLLADA_BUILDNAME}-1.4-d
    $ENV{COLLADA_DIR}/lib
    $ENV{COLLADA_DIR}/lib-dbg
    $ENV{COLLADA_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
    ${ACTUAL_3DPARTY_DIR}/lib
)

FIND_LIBRARY(COLLADA_STATIC_LIBRARY
    NAMES libcollada14dom21-s  libcollada14dom22-s libcollada14dom.a
    PATHS
    ${COLLADA_DOM_ROOT}/build/${COLLADA_BUILDNAME}-1.4
    $ENV{COLLADA_DIR}/build/${COLLADA_BUILDNAME}-1.4
    $ENV{COLLADA_DIR}/lib
    $ENV{COLLADA_DIR}/lib-dbg
    $ENV{COLLADA_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
    ${ACTUAL_3DPARTY_DIR}/lib
)

FIND_LIBRARY(COLLADA_STATIC_LIBRARY_DEBUG
    NAMES collada_dom-sd collada14dom-sd libcollada14dom21-sd libcollada14dom22-sd libcollada14dom-d.a
    PATHS
    ${COLLADA_DOM_ROOT}/build/${COLLADA_BUILDNAME}-1.4-d
    $ENV{COLLADA_DIR}/build/${COLLADA_BUILDNAME}-1.4-d
    $ENV{COLLADA_DIR}/lib
    $ENV{COLLADA_DIR}/lib-dbg
    $ENV{COLLADA_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /opt/local/Library/Frameworks #macports
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
    ${ACTUAL_3DPARTY_DIR}/lib
)

    # find extra libraries that the static linking requires

    FIND_PACKAGE(LibXml2)
    IF (LIBXML2_FOUND)
        SET(COLLADA_LIBXML_LIBRARY "${LIBXML2_LIBRARIES}" CACHE FILEPATH "" FORCE)
    ELSE(LIBXML2_FOUND)
        IF(WIN32)
            FIND_LIBRARY(COLLADA_LIBXML_LIBRARY
                NAMES libxml2
                PATHS
                ${COLLADA_DOM_ROOT}/external-libs/libxml2/win32/lib
                ${COLLADA_DOM_ROOT}/external-libs/libxml2/mingw/lib
                ${ACTUAL_3DPARTY_DIR}/lib
            )
        ENDIF(WIN32)
    ENDIF(LIBXML2_FOUND)

    FIND_PACKAGE(ZLIB)
    IF (ZLIB_FOUND)
        IF (ZLIB_LIBRARY_RELEASE)
            SET(COLLADA_ZLIB_LIBRARY "${ZLIB_LIBRARY_RELEASE}" CACHE FILEPATH "" FORCE)
        ELSE(ZLIB_LIBRARY_RELEASE)
            SET(COLLADA_ZLIB_LIBRARY "${ZLIB_LIBRARY}" CACHE FILEPATH "" FORCE)
        ENDIF(ZLIB_LIBRARY_RELEASE)
        IF (ZLIB_LIBRARY_DEBUG)
            SET(COLLADA_ZLIB_LIBRARY_DEBUG "${ZLIB_LIBRARY_DEBUG}" CACHE FILEPATH "" FORCE)
        ELSE(ZLIB_LIBRARY_DEBUG)
            SET(COLLADA_ZLIB_LIBRARY_DEBUG "${COLLADA_ZLIB_LIBRARY}" CACHE FILEPATH "" FORCE)
        ENDIF(ZLIB_LIBRARY_DEBUG)
    ELSE(ZLIB_FOUND)
        IF(WIN32)
            FIND_LIBRARY(COLLADA_ZLIB_LIBRARY
                NAMES zlib
                PATHS
                ${COLLADA_DOM_ROOT}/external-libs/libxml2/win32/lib
                ${COLLADA_DOM_ROOT}/external-libs/libxml2/mingw/lib
                ${ACTUAL_3DPARTY_DIR}/lib
            )
        ENDIF(WIN32)
    ENDIF(ZLIB_FOUND)

    FIND_LIBRARY(COLLADA_PCRECPP_LIBRARY
        NAMES pcrecpp
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mac
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_PCRECPP_LIBRARY_DEBUG
        NAMES pcrecpp-d pcrecppd
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mac
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_PCRE_LIBRARY
        NAMES pcre
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mac
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_PCRE_LIBRARY_DEBUG
        NAMES pcre-d pcred
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mac
        ${COLLADA_DOM_ROOT}/external-libs/pcre/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_MINIZIP_LIBRARY
        NAMES minizip
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/minizip/win32/lib
        ${COLLADA_DOM_ROOT}/external-libs/minizip/mac
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_MINIZIP_LIBRARY_DEBUG
        NAMES minizip-d minizipD
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/minizip/win32/lib
        ${COLLADA_DOM_ROOT}/external-libs/minizip/mac
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_BOOST_FILESYSTEM_LIBRARY
        NAMES libboost_filesystem boost_filesystem boost_filesystem-mt libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-1_54 libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-1_55 libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-1_58 boost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-1_62 boost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-1_63
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_BOOST_FILESYSTEM_LIBRARY_DEBUG
        NAMES libboost_filesystem-d boost_filesystem-d boost_filesystem-mt-d libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_54 libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_55 libboost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_58 boost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_62 boost_filesystem-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_63
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_BOOST_SYSTEM_LIBRARY
        NAMES libboost_system boost_system boost_system-mt libboost_system-${COLLADA_BOOST_BUILDNAME}-mt libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-1_54 libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-1_55  libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-1_58 boost_system-${COLLADA_BOOST_BUILDNAME}-mt-1_62 boost_system-${COLLADA_BOOST_BUILDNAME}-mt-1_63
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )

    FIND_LIBRARY(COLLADA_BOOST_SYSTEM_LIBRARY_DEBUG
        NAMES libboost_system-d boost_system-d boost_system-mt-d libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_54 libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_55 libboost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_58 boost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_62 boost_system-${COLLADA_BOOST_BUILDNAME}-mt-gd-1_63
        PATHS
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/${COLLADA_BUILDNAME}
        ${COLLADA_DOM_ROOT}/external-libs/boost/lib/mingw
        ${ACTUAL_3DPARTY_DIR}/lib
    )


SET(COLLADA_FOUND "NO")
IF(COLLADA_DYNAMIC_LIBRARY OR COLLADA_STATIC_LIBRARY)
    IF   (COLLADA_INCLUDE_DIR)

        SET(COLLADA_FOUND "YES")

        FIND_PATH(COLLADA_INCLUDE_DOMANY_DIR 1.4/dom/domAny.h
            ${COLLADA_INCLUDE_DIR}
        )

        IF (COLLADA_INCLUDE_DOMANY_DIR)
            SET(COLLADA_DOM_2_4_OR_LATER TRUE)
        ELSEIF()
            SET(COLLADA_DOM_2_4_OR_LATER FALSE)
        ENDIF()

        ENDIF()
ENDIF()
