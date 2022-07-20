#ifndef CSM_WOLRD_REFCOLLECTION_H
#define CSM_WOLRD_REFCOLLECTION_H

#include <map>
#include <string_view>

#include "../doc/stage.hpp"

#include "collection.hpp"
#include "ref.hpp"
#include "record.hpp"

namespace CSMWorld
{
    struct Cell;
    class UniversalId;

    template<>
    void Collection<CellRef, IdAccessor<CellRef> >::removeRows (int index, int count);

    template<>
    void Collection<CellRef, IdAccessor<CellRef> >::insertRecord (std::unique_ptr<RecordBase> record, int index,
        UniversalId::Type type);

    /// \brief References in cells
    class RefCollection : public Collection<CellRef>
    {
            Collection<Cell>& mCells;
            std::map<unsigned int, int> mRefIndex; // CellRef index keyed by CSMWorld::CellRef::mIdNum

            int mNextId;

            unsigned int extractIdNum(std::string_view id) const;

            int getIntIndex (unsigned int id) const;

            int searchId (unsigned int id) const;

        public:
            // MSVC needs the constructor for a class inheriting a template to be defined in header
            RefCollection (Collection<Cell>& cells)
              : mCells (cells), mNextId (0)
            {}

            void load (ESM::ESMReader& reader, int cellIndex, bool base,
                std::map<unsigned int, unsigned int>& cache, CSMDoc::Messages& messages);
            ///< Load a sequence of references.

            std::string getNewId();

            virtual void removeRows (int index, int count);

            virtual void appendBlankRecord (const std::string& id,
                                            UniversalId::Type type = UniversalId::Type_None);

            virtual void cloneRecord (const std::string& origin,
                                      const std::string& destination,
                                      const UniversalId::Type type);

            virtual int searchId(std::string_view id) const;

            virtual void appendRecord (std::unique_ptr<RecordBase> record,
                                       UniversalId::Type type = UniversalId::Type_None);

            virtual void insertRecord (std::unique_ptr<RecordBase> record,
                                       int index,
                                       UniversalId::Type type = UniversalId::Type_None);
    };
}

#endif
