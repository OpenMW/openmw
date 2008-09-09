# CMake build file list for OpenMW

cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(FindPkgConfig)
include(UsePkgConfig)

project(OpenMW D C CXX)

include_directories("${OpenMW_SOURCE_DIR}")

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

pkg_check_modules(Ogre OGRE)
if(NOT Ogre_FOUND)
    check_include_file_cxx(Ogre.h HAVE_OGRE_H)
    if(NOT HAVE_OGRE_H)
        message(FATAL_ERROR "Ogre.h not found!")
    endif()
    set(Ogre_LIBRARIES OgreMain)
else()
    include_directories(${Ogre_INCLUDE_DIRS})
    link_directories(${Ogre_LIBRARY_DIRS})
endif()

pkg_check_modules(OIS OIS)
if(NOT OIS_FOUND)
    check_include_file_cxx(OIS/OIS.h HAVE_OIS_OIS_H)
    if(NOT HAVE_OIS_OIS_H)
        message(FATAL_ERROR "OIS/OIS.h not found!")
    endif()
    set(OIS_LIBRARIES OIS)
else()
    include_directories(${OIS_INCLUDE_DIRS})
    link_directories(${OIS_LIBRARY_DIRS})
endif()

pkg_check_modules(FFmpeg libavcodec libavformat)
if(NOT FFmpeg_FOUND)
    check_include_file(ffmpeg/avcodec.h HAVE_FFMPEG_AVCODEC_H)
    check_include_file(ffmpeg/avformat.h HAVE_FFMPEG_AVFORMAT_H)
    if(NOT HAVE_FFMPEG_AVCODEC_H AND NOT HAVE_FFMPEG_AVFORMAT_H)
        message(FATAL_ERROR "FFmpeg not found!")
    endif()
    set(FFmpeg_LIBRARIES avformat avcodec)
else()
    include_directories(${FFmpeg_INCLUDE_DIRS})
    link_directories(${FFmpeg_LIBRARY_DIRS})
endif()

pkg_check_modules(OpenAL openal)
if(NOT OpenAL_FOUND)
    if(WIN32)
        set(OpenAL_LIBRARIES OpenAL32)
    else()
        set(OpenAL_LIBRARIES openal)
    endif()
else()
    include_directories(${OpenAL_INCLUDE_DIRS})
    link_directories(${OpenAL_LIBRARY_DIRS})
endif()


SET(OMWBASE_CXX_OBJS  ogre/cpp_ogre.cpp
                      sound/cpp_avcodec.cpp
)

set(OMWBASE_OBJS
                  util/reglist.d
                  util/regions.d
                  util/uniquename.d
                  util/random.d
                  core/inifile.d
                  core/resource.d
                  core/config.d
                  core/filefinder.d
                  core/memory.d
                  sound/sfx.d
                  sound/audio.d
                  sound/music.d
                  scene/soundlist.d
                  scene/player.d
                  scene/celldata.d
                  ogre/ogre.d
                  ogre/bindings.d
                  ogre/meshloader.d
                  bsa/bsafile.d
                  input/events.d
                  input/ois.d
                  input/keys.d
                  esm/loadrace.d
                  esm/loadappa.d
                  esm/loadbsgn.d
                  esm/loaddoor.d
                  esm/loadnpcc.d
                  esm/loadlocks.d
                  esm/loadacti.d
                  esm/loadgmst.d
                  esm/records.d
                  esm/loadscpt.d
                  esm/loadinfo.d
                  esm/loadcrea.d
                  esm/loadbody.d
                  esm/filereader.d
                  esm/loadcont.d
                  esm/loadfact.d
                  esm/loadregn.d
                  esm/loadltex.d
                  esm/loadalch.d
                  esm/loadarmo.d
                  esm/loadstat.d
                  esm/loadsoun.d
                  esm/loadspel.d
                  esm/loadskil.d
                  esm/loadingr.d
                  esm/loadclas.d
                  esm/loadnpc.d
                  esm/esmmain.d
                  esm/loadclot.d
                  esm/loadsscr.d
                  esm/loadlevlist.d
                  esm/loadbook.d
                  esm/listkeeper.d
                  esm/defs.d
                  esm/loadglob.d
                  esm/loadsndg.d
                  esm/loadligh.d
                  esm/imports.d
                  esm/loaddial.d
                  esm/loadmisc.d
                  esm/loadweap.d
                  esm/loadmgef.d
                  esm/loadcell.d
                  esm/loadcrec.d
                  esm/loadench.d
                  nif/property.d
                  nif/data.d
                  nif/controller.d
                  nif/nif.d
                  nif/base.d
                  nif/record.d
                  nif/controlled.d
                  nif/misc.d
                  nif/effect.d
                  nif/node.d
                  nif/extra.d
                  nif/niffile.d
                  monster/util/freelist.d
                  monster/util/aa.d
                  monster/util/list.d
                  monster/util/growarray.d
                  monster/util/string.d
)

ADD_LIBRARY(omwbase STATIC ${OMWBASE_CXX_OBJS} ${OMWBASE_OBJS})

ADD_EXECUTABLE(openmw openmw.d)
TARGET_LINK_LIBRARIES(openmw omwbase ${Ogre_LIBRARIES} ${OIS_LIBRARIES} ${OpenAL_LIBRARIES} ${FFmpeg_LIBRARIES})

ADD_EXECUTABLE(esmtool esmtool.d)
TARGET_LINK_LIBRARIES(esmtool omwbase ${Ogre_LIBRARIES} ${OIS_LIBRARIES} ${OpenAL_LIBRARIES} ${FFmpeg_LIBRARIES})


set(NIFTOOL_OBJS
                  nif/property.d
                  nif/data.d
                  nif/controller.d
                  nif/nif.d
                  nif/base.d
                  nif/record.d
                  nif/controlled.d
                  nif/misc.d
                  nif/effect.d
                  nif/node.d
                  nif/extra.d
                  nif/niffile.d
                  util/reglist.d
                  util/regions.d
                  util/uniquename.d
                  util/random.d
                  core/memory.d
                  monster/util/freelist.d
                  monster/util/aa.d
                  monster/util/list.d
                  monster/util/growarray.d
                  monster/util/string.d
)

ADD_LIBRARY(nifbase STATIC ${NIFTOOL_OBJS})

ADD_EXECUTABLE(niftool niftool.d)
TARGET_LINK_LIBRARIES(niftool nifbase)

ADD_EXECUTABLE(bsatool bsa/bsafile.d bsatool.d)
TARGET_LINK_LIBRARIES(bsatool nifbase)

ADD_EXECUTABLE(bored bored.d)
