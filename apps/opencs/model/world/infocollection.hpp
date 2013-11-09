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
        public:

            typedef std::map<std::string, int>::const_iterator MapConstIterator;

        private:

            void load (const Info& record, bool base);

            int getIndex (const std::string& id, const std::string& topic) const;
            ///< Return index for record \a id or -1 (if not present; deleted records are considered)
            ///
            /// \param id info ID without topic prefix

        public:

            virtual int getAppendIndex (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            void load (ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue);

            std::pair<MapConstIterator, MapConstIterator> getTopicRange (const std::string& topic)
                const;
            ///< Return iterators that point to the beginning and past the end of the range for
            /// the given topic.
    };
}

#endif