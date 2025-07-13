#ifndef CSM_WOLRD_REFCOLLECTION_H
#define CSM_WOLRD_REFCOLLECTION_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <apps/opencs/model/world/universalid.hpp>

#include "collection.hpp"
#include "record.hpp"
#include "ref.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMDoc
{
    class Messages;
}

namespace CSMWorld
{
    struct Cell;

    template <>
    void Collection<CellRef>::removeRows(int index, int count);

    template <>
    void Collection<CellRef>::insertRecord(std::unique_ptr<RecordBase> record, int index, UniversalId::Type type);

    /// \brief References in cells
    class RefCollection final : public Collection<CellRef>
    {
        Collection<Cell>& mCells;
        std::map<unsigned int, int> mRefIndex; // CellRef index keyed by CSMWorld::CellRef::mIdNum

        int mNextId;
        uint32_t mHighestUsedRefNum = 0;

        unsigned int extractIdNum(std::string_view id) const;

        uint32_t getNextRefNum();

        int getIntIndex(unsigned int id) const;

        int searchId(unsigned int id) const;

    public:
        // MSVC needs the constructor for a class inheriting a template to be defined in header
        RefCollection(Collection<Cell>& cells)
            : mCells(cells)
            , mNextId(0)
        {
        }

        void load(ESM::ESMReader& reader, int cellIndex, bool base, std::map<ESM::RefNum, unsigned int>& cache,
            CSMDoc::Messages& messages);
        ///< Load a sequence of references.

        std::string getNewId();

        void removeRows(int index, int count) override;

        void appendBlankRecord(const ESM::RefId& id, UniversalId::Type type = UniversalId::Type_None) override;

        void cloneRecord(
            const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type) override;

        int searchId(const ESM::RefId& id) const override;

        void appendRecord(std::unique_ptr<RecordBase> record, UniversalId::Type type = UniversalId::Type_None) override;

        void insertRecord(
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type = UniversalId::Type_None) override;
    };
}

#endif
