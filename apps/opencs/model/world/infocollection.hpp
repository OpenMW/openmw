#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include <components/esm/loadinfo.hpp>

#include "collection.hpp"

namespace CSMWorld
{
    class InfoCollection : public Collection<ESM::DialInfo, IdAccessor<ESM::DialInfo> >
    {
            void load (const ESM::DialInfo& record, bool base);

        public:

            void load (ESM::ESMReader& reader, bool base);
    };
}

#endif