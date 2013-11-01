#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include "collection.hpp"
#include "info.hpp"

namespace ESM
{
    class Dialogue;
}

namespace CSMWorld
{
    class InfoCollection : public Collection<Info, IdAccessor<Info> >
    {
            void load (const Info& record, bool base);

        public:

            void load (ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue);
    };
}

#endif