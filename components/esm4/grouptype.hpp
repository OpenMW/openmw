/*
  Copyright (C) 2015-2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef OPENMW_COMPONENTS_ESM4_GROUPTYPE_H
#define OPENMW_COMPONENTS_ESM4_GROUPTYPE_H

namespace ESM4
{
    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Groups
    enum GroupType
    {
        Grp_RecordType = 0,
        Grp_WorldChild = 1,
        Grp_InteriorCell = 2,
        Grp_InteriorSubCell = 3,
        Grp_ExteriorCell = 4,
        Grp_ExteriorSubCell = 5,
        Grp_CellChild = 6,
        Grp_TopicChild = 7,
        Grp_CellPersistentChild = 8,
        Grp_CellTemporaryChild = 9,
        Grp_CellVisibleDistChild = 10
    };
}

#endif // OPENMW_COMPONENTS_ESM4_GROUPTYPE_H
