#ifndef CSM_WOLRD_IDLIST_H
#define CSM_WOLRD_IDLIST_H

#include <map>

#include <components/esm/loadglob.hpp>

#include "idcollection.hpp"
#include "universalid.hpp"

class QAbstractTableModel;

namespace CSMWorld
{
    class Data
    {
            IdCollection<ESM::Global> mGlobals;
            std::map<UniversalId, QAbstractTableModel *> mModels;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

        public:

            Data();

            ~Data();

            const IdCollection<ESM::Global>& getGlobals() const;

            IdCollection<ESM::Global>& getGlobals();

            QAbstractTableModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \æ id, an exception is thrown.

            void merge();
            ///< Merge modified into base.
    };
}

#endif