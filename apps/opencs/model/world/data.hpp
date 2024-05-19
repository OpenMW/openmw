#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <QObject>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <apps/opencs/model/world/collection.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/debugprofile.hpp>
#include <components/esm3/filter.hpp>
#include <components/esm3/infoorder.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadsndg.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadsscr.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esm3/selectiongroup.hpp>
#include <components/files/multidircollection.hpp>
#include <components/misc/algorithm.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include "cell.hpp"
#include "idcollection.hpp"
#include "infocollection.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "metadata.hpp"
#include "nestedidcollection.hpp"
#include "nestedinfocollection.hpp"
#include "pathgrid.hpp"
#include "refcollection.hpp"
#include "refidcollection.hpp"
#include "resourcesmanager.hpp"
#include "universalid.hpp"
#ifndef Q_MOC_RUN
#include "subcellcollection.hpp"
#endif

class QAbstractItemModel;
class QModelIndex;

namespace Resource
{
    class ResourceSystem;
}

namespace VFS
{
    class Manager;
}

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
    class ActorAdapter;
    class CollectionBase;
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
        IdCollection<ESM::DebugProfile> mDebugProfiles;
        IdCollection<ESM::SelectionGroup> mSelectionGroups;
        IdCollection<ESM::SoundGenerator> mSoundGens;
        IdCollection<ESM::StartScript> mStartScripts;
        NestedInfoCollection mTopicInfos;
        InfoCollection mJournalInfos;
        NestedIdCollection<Cell> mCells;
        SubCellCollection<Pathgrid> mPathgrids;
        IdCollection<LandTexture> mLandTextures;
        IdCollection<Land> mLand;
        RefIdCollection mReferenceables;
        RefCollection mRefs;
        IdCollection<ESM::Filter> mFilters;
        Collection<MetaData> mMetaData;
        std::unique_ptr<ActorAdapter> mActorAdapter;
        std::vector<QAbstractItemModel*> mModels;
        std::map<UniversalId::Type, QAbstractItemModel*> mModelIndex;
        std::optional<ESM::ReadersCache> mReaders;
        const ESM::Dialogue* mDialogue; // last loaded dialogue
        bool mBase;
        bool mProject;
        std::map<ESM::RefId, std::map<ESM::RefNum, unsigned int>> mRefLoadCache;
        std::size_t mReaderIndex;

        Files::PathContainer mDataPaths;
        std::vector<std::string> mArchives;
        std::unique_ptr<VFS::Manager> mVFS;
        ResourcesManager mResourcesManager;
        std::shared_ptr<Resource::ResourceSystem> mResourceSystem;

        InfoOrderByTopic mJournalInfoOrder;
        InfoOrderByTopic mTopicInfoOrder;

        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

        void addModel(QAbstractItemModel* model, UniversalId::Type type, bool update = true);

        static void appendIds(std::vector<ESM::RefId>& ids, const CollectionBase& collection, bool listDeleted);
        ///< Append all IDs from collection to \a ids.

        static int count(RecordBase::State state, const CollectionBase& collection);

        void loadFallbackEntries();

    public:
        Data(ToUTF8::FromType encoding, const Files::PathContainer& dataPaths, const std::vector<std::string>& archives,
            const std::filesystem::path& resDir);

        ~Data() override;

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

        const IdCollection<ESM::SelectionGroup>& getSelectionGroups() const;

        IdCollection<ESM::SelectionGroup>& getSelectionGroups();

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
        const Resources& getResources(const UniversalId& id) const;

        const MetaData& getMetaData() const;

        void setMetaData(const MetaData& metaData);

        QAbstractItemModel* getTableModel(const UniversalId& id);
        ///< If no table model is available for \a id, an exception is thrown.
        ///
        /// \note The returned table may either be the model for the ID itself or the model that
        /// contains the record specified by the ID.

        const ActorAdapter* getActorAdapter() const;

        ActorAdapter* getActorAdapter();

        void merge();
        ///< Merge modified into base.

        int getTotalRecords(const std::vector<std::filesystem::path>& files); // for better loading bar

        int startLoading(const std::filesystem::path& path, bool base, bool project);
        ///< Begin merging content of a file into base or modified.
        ///
        /// \param project load project file instead of content file
        ///
        ///< \return estimated number of records

        bool continueLoading(CSMDoc::Messages& messages);
        ///< \return Finished?

        void finishLoading();

        bool hasId(const std::string& id) const;

        std::vector<ESM::RefId> getIds(bool listDeleted = true) const;
        ///< Return a sorted collection of all IDs that are not internal to the editor.
        ///
        /// \param listDeleted include deleted record in the list

        int count(RecordBase::State state) const;
        ///< Return number of top-level records with the given \a state.

    signals:

        void idListChanged();

        void assetTablesChanged();

    public slots:

        void assetsChanged();

    private slots:

        void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        void rowsChanged(const QModelIndex& parent, int start, int end);
    };
}

#endif
