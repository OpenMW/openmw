#ifndef OPENMW_ESSIMPORT_IMPORTJOUR_H
#define OPENMW_ESSIMPORT_IMPORTJOUR_H

#include <string>

namespace ESM
{
    struct ESMReader;
}

namespace ESSImport
{

    /// Journal
    struct JOUR
    {
        // The entire journal, in HTML
        std::string mText;

        void load(ESM::ESMReader& esm);
    };

}

#endif
