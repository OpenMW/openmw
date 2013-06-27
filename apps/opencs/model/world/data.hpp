#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <components/esm/loadglob.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadsoun.hpp>
#include <components/esm/loadscpt.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/loadbsgn.hpp>
#include <components/esm/loadspel.hpp>

#include "idcollection.hpp"
#include "universalid.hpp"
#include "cell.hpp"
#include "refidcollection.hpp"

class QAbstractItemModel;

namespace CSMWorld
{
    class Data
    {
            IdCollection<ESM::Global> mGlobals;
            IdCollection<ESM::GameSetting> mGmsts;
            IdCollection<ESM::Skill> mSkills;
            IdCollection<ESM::Class> mClasses;
            IdCollection<ESM::Faction> mFactions;
            IdCollection<ESM::Race> mRaces;
            IdCollection<ESM::Sound> mSounds;
            IdCollection<ESM::Script> mScripts;
            IdCollection<ESM::Region> mRegions;
            IdCollection<ESM::BirthSign> mBirthsigns;
            IdCollection<ESM::Spell> mSpells;
            IdCollection<Cell> mCells;
            RefIdCollection mReferenceables;
            std::vector<QAbstractItemModel *> mModels;
            std::map<UniversalId::Type, QAbstractItemModel *> mModelIndex;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractItemModel *model, UniversalId::Type type1,
                UniversalId::Type type2 = UniversalId::Type_None);

        public:

            Data();

            ~Data();

            const IdCollection<ESM::Global>& getGlobals() const;

            IdCollection<ESM::Global>& getGlobals();

            const IdCollection<ESM::GameSetting>& getGmsts() const;

            IdCollection<ESM::GameSetting>& getGmsts();

            const IdCollection<ESM::Skill>& getSkills() const;

            IdCollection<ESM::Skill>& getSkills();

            const IdCollection<ESM::Class>& getClasses() const;

            IdCollection<ESM::Class>& getClasses();

            const IdCollection<ESM::Faction>& getFactions() const;

            IdCollection<ESM::Faction>& getFactions();

            const IdCollection<ESM::Race>& getRaces() const;

            IdCollection<ESM::Race>& getRaces();

            const IdCollection<ESM::Sound>& getSounds() const;

            IdCollection<ESM::Sound>& getSounds();

            const IdCollection<ESM::Script>& getScripts() const;

            IdCollection<ESM::Script>& getScripts();

            const IdCollection<ESM::Region>& getRegions() const;

            IdCollection<ESM::Region>& getRegions();

            const IdCollection<ESM::BirthSign>& getBirthsigns() const;

            IdCollection<ESM::BirthSign>& getBirthsigns();

            const IdCollection<ESM::Spell>& getSpells() const;

            IdCollection<ESM::Spell>& getSpells();

            const IdCollection<Cell>& getCells() const;

            IdCollection<Cell>& getCells();

            const RefIdCollection& getReferenceables() const;

            RefIdCollection& getReferenceables();

            QAbstractItemModel *getTableModel (const UniversalId& id);
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