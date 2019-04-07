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
#include <components/esm/loadench.hpp>
#include <components/esm/loadbody.hpp>
#include <components/esm/loadsndg.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadsscr.hpp>
#include <components/esm/debugprofile.hpp>
#include <components/esm/filter.hpp>

#include <components/resource/resourcesystem.hpp>

#include <components/files/multidircollection.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

#include "actoradapter.hpp"
#include "idcollection.hpp"
#include "nestedidcollection.hpp"
#include "universalid.hpp"
#include "cell.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "refidcollection.hpp"
#include "refcollection.hpp"
#include "infocollection.hpp"
#include "nestedinfocollection.hpp"
#include "pathgrid.hpp"
#include "resourcesmanager.hpp"
#include "metadata.hpp"
#ifndef Q_MOC_RUN
#include "subcellcollection.hpp"
#endif

class QAbstractItemModel;

namespace VFS
{
    class Manager;
}

namespace Fallback
{
    class Map;
}

namespace ESM
{
    class ESMReader;
    struct Dialogue;
}

namespace CSMWorld
{
    class ResourcesManager;
    class Resources;

    class Data : public QObject
    {
            Q_OBJECT

            ToUTF8::Utf8Encoder mEncoder;
            IdCollection<ESM::Global> mGlobals;
            IdCollection<ESM::GameSetting> mGmsts;
            IdCollection<ESM::Skill> mSkills;
            IdCollection<ESM::Class> mClasses;
            NestedIdCollection<ESM::Faction> mFactions;
            NestedIdCollection<ESM::Race> mRaces;
            IdCollection<ESM::Sound> mSounds;
            IdCollection<ESM::Script> mScripts;
            NestedIdCollection<ESM::Region> mRegions;
            NestedIdCollection<ESM::BirthSign> mBirthsigns;
            NestedIdCollection<ESM::Spell> mSpells;
            IdCollection<ESM::Dialogue> mTopics;
            IdCollection<ESM::Dialogue> mJournals;
            NestedIdCollection<ESM::Enchantment> mEnchantments;
            IdCollection<ESM::BodyPart> mBodyParts;
            IdCollection<ESM::MagicEffect> mMagicEffects;
            SubCellCollection<Pathgrid> mPathgrids;
            IdCollection<ESM::DebugProfile> mDebugProfiles;
            IdCollection<ESM::SoundGenerator> mSoundGens;
            IdCollection<ESM::StartScript> mStartScripts;
            NestedInfoCollection mTopicInfos;
            InfoCollection mJournalInfos;
            NestedIdCollection<Cell> mCells;
            IdCollection<LandTexture> mLandTextures;
            IdCollection<Land> mLand;
            RefIdCollection mReferenceables;
            RefCollection mRefs;
            IdCollection<ESM::Filter> mFilters;
            Collection<MetaData> mMetaData;
            std::unique_ptr<ActorAdapter> mActorAdapter;
            std::vector<QAbstractItemModel *> mModels;
            std::map<UniversalId::Type, QAbstractItemModel *> mModelIndex;
            ESM::ESMReader *mReader;
            const ESM::Dialogue *mDialogue; // last loaded dialogue
            bool mBase;
            bool mProject;
            std::map<std::string, std::map<ESM::RefNum, std::string> > mRefLoadCache;
            int mReaderIndex;

            bool mFsStrict;
            Files::PathContainer mDataPaths;
            std::vector<std::string> mArchives;
            std::unique_ptr<VFS::Manager> mVFS;
            ResourcesManager mResourcesManager;
            std::shared_ptr<Resource::ResourceSystem> mResourceSystem;

            std::vector<std::shared_ptr<ESM::ESMReader> > mReaders;

            std::map<std::string, int> mContentFileNames;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractItemModel *model, UniversalId::Type type,
                bool update = true);

            static void appendIds (std::vector<std::string>& ids, const CollectionBase& collection,
                bool listDeleted);
            ///< Append all IDs from collection to \a ids.

            static int count (RecordBase::State state, const CollectionBase& collection);

            void loadFallbackEntries();

        public:

            Data (ToUTF8::FromType encoding, bool fsStrict, const Files::PathContainer& dataPaths,
                const std::vector<std::string>& archives, const boost::filesystem::path& resDir);

            virtual ~Data();

            const VFS::Manager* getVFS() const;

            std::shared_ptr<Resource::ResourceSystem> getResourceSystem();

            std::shared_ptr<const Resource::ResourceSystem> getResourceSystem() const;

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

            const IdCollection<ESM::Filter>& getFilters() const;

            IdCollection<ESM::Filter>& getFilters();

            const IdCollection<ESM::Enchantment>& getEnchantments() const;

            IdCollection<ESM::Enchantment>& getEnchantments();

            const IdCollection<ESM::BodyPart>& getBodyParts() const;

            IdCollection<ESM::BodyPart>& getBodyParts();

            const IdCollection<ESM::DebugProfile>& getDebugProfiles() const;

            IdCollection<ESM::DebugProfile>& getDebugProfiles();

            const IdCollection<CSMWorld::Land>& getLand() const;

            IdCollection<CSMWorld::Land>& getLand();

            const IdCollection<CSMWorld::LandTexture>& getLandTextures() const;

            IdCollection<CSMWorld::LandTexture>& getLandTextures();

            const IdCollection<ESM::SoundGenerator>& getSoundGens() const;

            IdCollection<ESM::SoundGenerator>& getSoundGens();

            const IdCollection<ESM::MagicEffect>& getMagicEffects() const;

            IdCollection<ESM::MagicEffect>& getMagicEffects();

            const SubCellCollection<Pathgrid>& getPathgrids() const;

            SubCellCollection<Pathgrid>& getPathgrids();

            const IdCollection<ESM::StartScript>& getStartScripts() const;

            IdCollection<ESM::StartScript>& getStartScripts();

            /// Throws an exception, if \a id does not match a resources list.
            const Resources& getResources (const UniversalId& id) const;

            const MetaData& getMetaData() const;

            void setMetaData (const MetaData& metaData);

            QAbstractItemModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \a id, an exception is thrown.
            ///
            /// \note The returned table may either be the model for the ID itself or the model that
            /// contains the record specified by the ID.

            const ActorAdapter* getActorAdapter() const;

            ActorAdapter* getActorAdapter();

            void merge();
            ///< Merge modified into base.

            int startLoading (const boost::filesystem::path& path, bool base, bool project);
            ///< Begin merging content of a file into base or modified.
            ///
            /// \param project load project file instead of content file
            ///
            ///< \return estimated number of records

            bool continueLoading (CSMDoc::Messages& messages);
            ///< \return Finished?

            bool hasId (const std::string& id) const;

            std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs that are not internal to the editor.
            ///
            /// \param listDeleted include deleted record in the list

            int count (RecordBase::State state) const;
            ///< Return number of top-level records with the given \a state.

        signals:

            void idListChanged();

            void assetTablesChanged();

        private slots:

            void assetsChanged();

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsChanged (const QModelIndex& parent, int start, int end);
    };
}

#endif
