include_directories(./linux)

add_definitions(-DPAWN_CELL_SIZE=64)

set(source_amx
        amxexec_gcc.c
        amxfile.c
        fpattern.c
        minIni.c
        amxcons.c
        amxcore.c
        amxstring.c
        amxtime.c
        amxfloat.c
        amxaux.c
        amx.c
        linux/binreloc.c
        linux/getch.c
        )

source_group(amx FILES ${source_amx})

add_library(amx STATIC ${source_amx} )
