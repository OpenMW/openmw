#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include "collection.hpp"
#include "info.hpp"

namespace ESM
{
    struct Dialogue;
}

namespace CSMWorld
{
    class InfoCollection : public Collection<Info, IdAccessor<Info> >
    {
        public:

            typedef std::vector<Record<Info> >::const_iterator RecordConstIterator;
            typedef std::pair<RecordConstIterator, RecordConstIterator> Range;

        private:

            void load (const Info& record, bool base);

            int getInfoIndex (const std::string& id, const std::string& topic) const;
            ///< Return index for record \a id or -1 (if not present; deleted records are considered)
            ///
            /// \param id info ID without topic prefix

        public:

            int getAppendIndex (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None) const override;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            bool reorderRows (int baseIndex, const std::vector<int>& newOrder) override;
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            void load (ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue);

            Range getTopicRange (const std::string& topic) const;
            ///< Return iterators that point to the beginning and past the end of the range for
            /// the given topic.

            void removeDialogueInfos(const std::string& dialogueId);
    };
}

#endif
