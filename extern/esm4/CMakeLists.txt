#
#  Copyright (C) 2016-2019 cc9cii
#
#  This software is provided 'as-is', without any express or implied
#  warranty.  In no event will the authors be held liable for any damages
#  arising from the use of this software.
#
#  Permission is granted to anyone to use this software for any purpose,
#  including commercial applications, and to alter it and redistribute it
#  freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not
#     claim that you wrote the original software. If you use this software
#     in a product, an acknowledgment in the product documentation would be
#     appreciated but is not required.
#  2. Altered source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
#  3. This notice may not be removed or altered from any source distribution.
#
#  cc9cii cc9c@iinet.net.au
#

set(ESM4_LIBRARY "esm4")

set(ESM4_SOURCE_FILES
    common.cpp
    tes4.cpp
    navi.cpp
    wrld.cpp
    navm.cpp
    land.cpp
    ltex.cpp
    cell.cpp
    regn.cpp
    stat.cpp
    refr.cpp
    anio.cpp
    cont.cpp
    misc.cpp
    acti.cpp
    armo.cpp
    npc_.cpp
    flor.cpp
    gras.cpp
    tree.cpp
    ligh.cpp
    achr.cpp
    book.cpp
    furn.cpp
    soun.cpp
    weap.cpp
    door.cpp
    clot.cpp
    alch.cpp
    ammo.cpp
    appa.cpp
    ingr.cpp
    sgst.cpp
    slgm.cpp
    keym.cpp
    hair.cpp
    eyes.cpp
    crea.cpp
    lvlc.cpp
    lvli.cpp
    acre.cpp
    idle.cpp
    mato.cpp
    sbsp.cpp
    race.cpp
    clas.cpp
    lvln.cpp
    idlm.cpp
    mstt.cpp
    txst.cpp
    scrl.cpp
    arma.cpp
    hdpt.cpp
    term.cpp
    tact.cpp
    note.cpp
    bptd.cpp
    scpt.cpp
    formid.cpp
    reader.cpp
)

add_library(${ESM4_LIBRARY} STATIC ${ESM4_SOURCE_FILES})
set(ESM4_LIBRARIES ${ESM4_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(ESM4_LIBRARIES ${ESM4_LIBRARIES} PARENT_SCOPE)
