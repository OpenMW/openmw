#ifndef COMPONENTS_ESM_FILTER_H
#define COMPONENTS_ESM_FILTER_H

#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct Filter
    {
        static unsigned int sRecordId;

        std::string mId;

        std::string mDescription;

        std::string mFilter;

        void load (ESMReader& esm);
        void save (ESMWriter& esm) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}

#endif
