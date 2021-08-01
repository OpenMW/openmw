#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include <unordered_map>

#include "collection.hpp"
#include "info.hpp"

namespace ESM
{
    struct Dialogue;
}

namespace CSMWorld
{
    template<>
    void Collection<Info, IdAccessor<Info> >::removeRows (int index, int count);

    template<>
    void Collection<Info, IdAccessor<Info> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type);

    template<>
    bool Collection<Info, IdAccessor<Info> >::reorderRowsImp (int baseIndex,
        const std::vector<int>& newOrder);

    class InfoCollection : public Collection<Info, IdAccessor<Info> >
    {
        public:

            typedef std::vector<std::unique_ptr<Record<Info> > >::const_iterator RecordConstIterator;
            typedef std::pair<RecordConstIterator, RecordConstIterator> Range;

        private:

            // The general strategy is to keep the records in Collection kept in order (within
            // a topic group) while the index lookup maps are not ordered.  It is assumed that
            // each topic has a small number of infos, which allows the use of vectors for
            // iterating through them without too much penalty.
            //
            // NOTE: topic string as well as id string are stored in lower case.
            std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > mInfoIndex;

            void load (const Info& record, bool base);

            int getInfoIndex (const std::string& id, const std::string& topic) const;
            ///< Return index for record \a id or -1 (if not present; deleted records are considered)
            ///
            /// \param id info ID without topic prefix
            //
            /// \attention id and topic are assumed to be in lower case

        public:

            int getInsertIndex (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None,
                RecordBase *record = nullptr) const override;
            ///< \param type Will be ignored, unless the collection supports multiple record types
            ///
            /// Works like getAppendIndex unless an overloaded method uses the record pointer
            /// to get additional info about the record that results in an alternative index.

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

            void removeRows (int index, int count) override;

            void appendBlankRecord (const std::string& id,
                                            UniversalId::Type type = UniversalId::Type_None) override;

            int searchId (const std::string& id) const override;

            void appendRecord (std::unique_ptr<RecordBase> record,
                UniversalId::Type type = UniversalId::Type_None) override;

            void insertRecord (std::unique_ptr<RecordBase> record,
                                       int index,
                                       UniversalId::Type type = UniversalId::Type_None) override;
    };
}

#endif
