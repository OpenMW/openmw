project (Components)

# source files

add_component_dir (settings
    settings
    )

add_component_dir (nifoverrides
    nifoverrides
    )

add_component_dir (bsa
    bsa_archive bsa_file
    )

add_component_dir (nif
    controlled effect nif_types record controller extra node record_ptr data nif_file property
    )

add_component_dir (nifogre
    ogre_nif_loader
    )

add_component_dir (nifbullet
    bullet_nif_loader
    )

add_component_dir (to_utf8
    to_utf8
    )

add_component_dir (file_finder
    file_finder filename_less search
    )

add_component_dir (esm
    attr defs esmcommon esmreader esmwriter loadacti loadalch loadappa loadarmo loadbody loadbook loadbsgn loadcell
    loadclas loadclot loadcont loadcrea loadcrec loaddial loaddoor loadench loadfact loadglob loadgmst
    loadinfo loadingr loadland loadlevlist loadligh loadlocks loadltex loadmgef loadmisc loadnpcc
    loadnpc loadpgrd loadrace loadregn loadscpt loadskil loadsndg loadsoun loadspel loadsscr loadstat
    loadweap records aipackage effectlist spelllist
    )

add_component_dir (misc
    slice_array stringops
    )

add_component_dir (files
    linuxpath windowspath macospath fixedpath multidircollection collections fileops configurationmanager
    filelibrary ogreplugin constrainedfiledatastream lowlevelfile
    )

add_component_dir (compiler
    context controlparser errorhandler exception exprparser extensions fileparser generator
    lineparser literals locals output parser scanner scriptparser skipparser streamerrorhandler
    stringparser tokenloc
    )

add_component_dir (interpreter
    context controlopcodes genericopcodes installopcodes interpreter localopcodes mathopcodes
    miscopcodes opcodes runtime scriptopcodes spatialopcodes types defines
    )

add_component_dir (translation
    translation
    )

include_directories(${BULLET_INCLUDE_DIRS})

add_library(components STATIC ${COMPONENT_FILES})

target_link_libraries(components ${Boost_LIBRARIES} ${OGRE_LIBRARIES})

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(components ${CMAKE_THREAD_LIBS_INIT})
endif()


# Make the variable accessible for other subdirectories
set(COMPONENT_FILES ${COMPONENT_FILES} PARENT_SCOPE)
