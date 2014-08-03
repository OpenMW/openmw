#ifndef COMPONENTS_ESM_DEBUGPROFILE_H
#define COMPONENTS_ESM_DEBUGPROFILE_H

#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct DebugProfile
    {
        static unsigned int sRecordId;

        std::string mId;

        std::string mDescription;

        std::string mScript;

        bool mDefault;

        bool mBypassNewGame;

        void load (ESMReader& esm);
        void save (ESMWriter& esm) const;

        /// Set record to default state (does not touch the ID).
        void blank();
    };
}

#endif
