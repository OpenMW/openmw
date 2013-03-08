#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <components/esm/loadglob.hpp>
#include <components/esm/loadgmst.hpp>

#include "idcollection.hpp"
#include "universalid.hpp"

class QAbstractTableModel;

namespace CSMWorld
{
    class Data
    {
            IdCollection<ESM::Global> mGlobals;
            IdCollection<ESM::GameSetting> mGmsts;
            std::vector<QAbstractTableModel *> mModels;
            std::map<UniversalId::Type, QAbstractTableModel *> mModelIndex;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractTableModel *model, UniversalId::Type type1,
                UniversalId::Type type2 = UniversalId::Type_None);

        public:

            Data();

            ~Data();

            const IdCollection<ESM::Global>& getGlobals() const;

            IdCollection<ESM::Global>& getGlobals();

            const IdCollection<ESM::GameSetting>& getGmsts() const;

            IdCollection<ESM::GameSetting>& getGmsts();

            QAbstractTableModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \a id, an exception is thrown.
            ///
            /// \note The returned table may either be the model for the ID itself or the model that
            /// contains the record specified by the ID.

            void merge();
            ///< Merge modified into base.

            void loadFile (const boost::filesystem::path& path, bool base);
            ///< Merging content of a file into base or modified.
    };
}

#endif