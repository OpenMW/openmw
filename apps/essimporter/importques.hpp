#ifndef OPENMW_ESSIMPORT_IMPORTQUES_H
#define OPENMW_ESSIMPORT_IMPORTQUES_H

#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    /// State for a quest
    /// Presumably this record only exists when Tribunal is installed,
    /// since pre-Tribunal there weren't any quest names in the data files.
    struct QUES
    {
        std::string mName; // NAME, should be assigned from outside as usual
        std::vector<std::string> mInfo; // list of journal entries for the quest

        void load(ESM::ESMReader& esm);
    };

}

#endif
