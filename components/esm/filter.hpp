#ifndef COMPONENTS_ESM_FILTER_H
#define COMPONENTS_ESM_FILTER_H

#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct Filter
    {
        std::string mId;

        std::string mFilter;

        void load (ESMReader& esm);
        void save (ESMWriter& esm);

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}

#endif
