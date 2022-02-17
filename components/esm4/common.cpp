/*
  Copyright (C) 2015-2016, 2018, 2021 cc9cii

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
#include "common.hpp"

#include <sstream>
#include <algorithm>
#include <stdexcept>

#include <string>

#include "formid.hpp"

namespace ESM4
{
    const char *sGroupType[] =
    {
        "Record Type", "World Child", "Interior Cell", "Interior Sub Cell", "Exterior Cell",
        "Exterior Sub Cell", "Cell Child", "Topic Child", "Cell Persistent Child",
        "Cell Temporary Child", "Cell Visible Dist Child", "Unknown"
    };

    std::string printLabel(const GroupLabel& label, const std::uint32_t type)
    {
        std::ostringstream ss;
        ss << std::string(sGroupType[std::min(type, (uint32_t)11)]); // avoid out of range

        switch (type)
        {
            case ESM4::Grp_RecordType:
            {
                ss << ": " << std::string((char*)label.recordType, 4);
                break;
            }
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            {
                //short x, y;
                //y = label & 0xff;
                //x = (label >> 16) & 0xff;
                ss << ": grid (x, y) " << std::dec << label.grid[1] << ", " << label.grid[0];

                break;
            }
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            {
                ss << ": block 0x" << std::hex << label.value;
                break;
            }
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_CellChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellTemporaryChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                ss << ": FormId 0x" << formIdToString(label.value);
                break;
            }
            default:
                break;
        }

        return ss.str();
    }

    void gridToString(std::int16_t x, std::int16_t y, std::string& str)
    {
        char buf[6+6+2+1]; // longest signed 16 bit number is 6 characters (-32768)
        int res = snprintf(buf, 6+6+2+1, "#%d %d", x, y);
        if (res > 0 && res < 6+6+2+1)
            str.assign(buf);
        else
            throw std::runtime_error("possible buffer overflow while converting grid");
    }
}
