#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <QObject>
#include <QModelIndex>

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
#include <components/esm/loaddial.hpp>

#include "../filter/filter.hpp"

#include "idcollection.hpp"
#include "universalid.hpp"
#include "cell.hpp"
#include "refidcollection.hpp"
#include "refcollection.hpp"
#include "infocollection.hpp"

class QAbstractItemModel;

namespace CSMWorld
{
    class Data : public QObject
    {
            Q_OBJECT

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
            IdCollection<ESM::Dialogue> mTopics;
            IdCollection<ESM::Dialogue> mJournals;
            InfoCollection mTopicInfos;
            InfoCollection mJournalInfos;
            IdCollection<Cell> mCells;
            RefIdCollection mReferenceables;
            RefCollection mRefs;
            IdCollection<CSMFilter::Filter> mFilters;
            std::vector<QAbstractItemModel *> mModels;
            std::map<UniversalId::Type, QAbstractItemModel *> mModelIndex;
            std::string mAuthor;
            std::string mDescription;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractItemModel *model, UniversalId::Type type1,
                UniversalId::Type type2 = UniversalId::Type_None, bool update = true);

            static void appendIds (std::vector<std::string>& ids, const CollectionBase& collection,
                bool listDeleted);
            ///< Append all IDs from collection to \a ids.

            static int count (RecordBase::State state, const CollectionBase& collection);

        public:

            Data();

            virtual ~Data();

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

            const IdCollection<ESM::Dialogue>& getTopics() const;

            IdCollection<ESM::Dialogue>& getTopics();

            const IdCollection<ESM::Dialogue>& getJournals() const;

            IdCollection<ESM::Dialogue>& getJournals();

            const InfoCollection& getTopicInfos() const;

            InfoCollection& getTopicInfos();

            const InfoCollection& getJournalInfos() const;

            InfoCollection& getJournalInfos();

            const IdCollection<Cell>& getCells() const;

            IdCollection<Cell>& getCells();

            const RefIdCollection& getReferenceables() const;

            RefIdCollection& getReferenceables();

            const RefCollection& getReferences() const;

            RefCollection& getReferences();

            const IdCollection<CSMFilter::Filter>& getFilters() const;

            IdCollection<CSMFilter::Filter>& getFilters();

            QAbstractItemModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \a id, an exception is thrown.
            ///
            /// \note The returned table may either be the model for the ID itself or the model that
            /// contains the record specified by the ID.

            void merge();
            ///< Merge modified into base.

            void loadFile (const boost::filesystem::path& path, bool base, bool project);
            ///< Merging content of a file into base or modified.
            ///
            /// \param project load project file instead of content file

            bool hasId (const std::string& id) const;

            std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs that are not internal to the editor.
            ///
            /// \param listDeleted include deleted record in the list

            int count (RecordBase::State state) const;
            ///< Return number of top-level records with the given \a state.

            void setDescription (const std::string& description);

            std::string getDescription() const;

            void setAuthor (const std::string& author);

            std::string getAuthor() const;

        signals:

            void idListChanged();

        private slots:

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsChanged (const QModelIndex& parent, int start, int end);
    };
}

#endif