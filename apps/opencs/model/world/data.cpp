
#include "data.hpp"

#include <stdexcept>
#include <algorithm>

#include <QAbstractItemModel>

#include <components/esm/esmreader.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/loadglob.hpp>
#include <components/esm/cellref.hpp>

#include "idtable.hpp"
#include "columnimp.hpp"
#include "regionmap.hpp"
#include "columns.hpp"
#include "resourcesmanager.hpp"
#include "resourcetable.hpp"

void CSMWorld::Data::addModel (QAbstractItemModel *model, UniversalId::Type type, bool update)
{
    mModels.push_back (model);
    mModelIndex.insert (std::make_pair (type, model));

    UniversalId::Type type2 = UniversalId::getParentType (type);

    if (type2!=UniversalId::Type_None)
        mModelIndex.insert (std::make_pair (type2, model));

    if (update)
    {
        connect (model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (dataChanged (const QModelIndex&, const QModelIndex&)));
        connect (model, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
            this, SLOT (rowsChanged (const QModelIndex&, int, int)));
        connect (model, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
            this, SLOT (rowsChanged (const QModelIndex&, int, int)));
    }
}

void CSMWorld::Data::appendIds (std::vector<std::string>& ids, const CollectionBase& collection,
    bool listDeleted)
{
    std::vector<std::string> ids2 = collection.getIds (listDeleted);

    ids.insert (ids.end(), ids2.begin(), ids2.end());
}

int CSMWorld::Data::count (RecordBase::State state, const CollectionBase& collection)
{
    int number = 0;

    for (int i=0; i<collection.getSize(); ++i)
        if (collection.getRecord (i).mState==state)
            ++number;

    return number;
}

CSMWorld::Data::Data (ToUTF8::FromType encoding, const ResourcesManager& resourcesManager)
: mEncoder (encoding), mRefs (mCells), mResourcesManager (resourcesManager), mReader (0),
  mDialogue (0)
{
    mGlobals.addColumn (new StringIdColumn<ESM::Global>);
    mGlobals.addColumn (new RecordStateColumn<ESM::Global>);
    mGlobals.addColumn (new FixedRecordTypeColumn<ESM::Global> (UniversalId::Type_Global));
    mGlobals.addColumn (new VarTypeColumn<ESM::Global> (ColumnBase::Display_GlobalVarType));
    mGlobals.addColumn (new VarValueColumn<ESM::Global>);

    mGmsts.addColumn (new StringIdColumn<ESM::GameSetting>);
    mGmsts.addColumn (new RecordStateColumn<ESM::GameSetting>);
    mGmsts.addColumn (new FixedRecordTypeColumn<ESM::GameSetting> (UniversalId::Type_Gmst));
    mGmsts.addColumn (new FixedRecordTypeColumn<ESM::GameSetting> (UniversalId::Type_Gmst));
    mGmsts.addColumn (new VarTypeColumn<ESM::GameSetting> (ColumnBase::Display_GmstVarType));
    mGmsts.addColumn (new VarValueColumn<ESM::GameSetting>);

    mSkills.addColumn (new StringIdColumn<ESM::Skill>);
    mSkills.addColumn (new RecordStateColumn<ESM::Skill>);
    mSkills.addColumn (new FixedRecordTypeColumn<ESM::Skill> (UniversalId::Type_Skill));
    mSkills.addColumn (new AttributeColumn<ESM::Skill>);
    mSkills.addColumn (new SpecialisationColumn<ESM::Skill>);
    for (int i=0; i<4; ++i)
        mSkills.addColumn (new UseValueColumn<ESM::Skill> (i));
    mSkills.addColumn (new DescriptionColumn<ESM::Skill>);

    mClasses.addColumn (new StringIdColumn<ESM::Class>);
    mClasses.addColumn (new RecordStateColumn<ESM::Class>);
    mClasses.addColumn (new FixedRecordTypeColumn<ESM::Class> (UniversalId::Type_Class));
    mClasses.addColumn (new NameColumn<ESM::Class>);
    mClasses.addColumn (new AttributesColumn<ESM::Class> (0));
    mClasses.addColumn (new AttributesColumn<ESM::Class> (1));
    mClasses.addColumn (new SpecialisationColumn<ESM::Class>);
    for (int i=0; i<5; ++i)
        mClasses.addColumn (new SkillsColumn<ESM::Class> (i, true, true));
    for (int i=0; i<5; ++i)
        mClasses.addColumn (new SkillsColumn<ESM::Class> (i, true, false));
    mClasses.addColumn (new PlayableColumn<ESM::Class>);
    mClasses.addColumn (new DescriptionColumn<ESM::Class>);

    mFactions.addColumn (new StringIdColumn<ESM::Faction>);
    mFactions.addColumn (new RecordStateColumn<ESM::Faction>);
    mFactions.addColumn (new FixedRecordTypeColumn<ESM::Faction> (UniversalId::Type_Faction));
    mFactions.addColumn (new NameColumn<ESM::Faction>);
    mFactions.addColumn (new AttributesColumn<ESM::Faction> (0));
    mFactions.addColumn (new AttributesColumn<ESM::Faction> (1));
    mFactions.addColumn (new HiddenColumn<ESM::Faction>);
    for (int i=0; i<7; ++i)
        mFactions.addColumn (new SkillsColumn<ESM::Faction> (i));

    mRaces.addColumn (new StringIdColumn<ESM::Race>);
    mRaces.addColumn (new RecordStateColumn<ESM::Race>);
    mRaces.addColumn (new FixedRecordTypeColumn<ESM::Race> (UniversalId::Type_Race));
    mRaces.addColumn (new NameColumn<ESM::Race>);
    mRaces.addColumn (new DescriptionColumn<ESM::Race>);
    mRaces.addColumn (new FlagColumn<ESM::Race> (Columns::ColumnId_Playable, 0x1));
    mRaces.addColumn (new FlagColumn<ESM::Race> (Columns::ColumnId_BeastRace, 0x2));
    mRaces.addColumn (new WeightHeightColumn<ESM::Race> (true, true));
    mRaces.addColumn (new WeightHeightColumn<ESM::Race> (true, false));
    mRaces.addColumn (new WeightHeightColumn<ESM::Race> (false, true));
    mRaces.addColumn (new WeightHeightColumn<ESM::Race> (false, false));

    mSounds.addColumn (new StringIdColumn<ESM::Sound>);
    mSounds.addColumn (new RecordStateColumn<ESM::Sound>);
    mSounds.addColumn (new FixedRecordTypeColumn<ESM::Sound> (UniversalId::Type_Sound));
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_Volume));
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_MinRange));
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_MaxRange));
    mSounds.addColumn (new SoundFileColumn<ESM::Sound>);

    mScripts.addColumn (new StringIdColumn<ESM::Script>);
    mScripts.addColumn (new RecordStateColumn<ESM::Script>);
    mScripts.addColumn (new FixedRecordTypeColumn<ESM::Script> (UniversalId::Type_Script));
    mScripts.addColumn (new ScriptColumn<ESM::Script>);

    mRegions.addColumn (new StringIdColumn<ESM::Region>);
    mRegions.addColumn (new RecordStateColumn<ESM::Region>);
    mRegions.addColumn (new FixedRecordTypeColumn<ESM::Region> (UniversalId::Type_Region));
    mRegions.addColumn (new NameColumn<ESM::Region>);
    mRegions.addColumn (new MapColourColumn<ESM::Region>);
    mRegions.addColumn (new SleepListColumn<ESM::Region>);

    mBirthsigns.addColumn (new StringIdColumn<ESM::BirthSign>);
    mBirthsigns.addColumn (new RecordStateColumn<ESM::BirthSign>);
    mBirthsigns.addColumn (new FixedRecordTypeColumn<ESM::BirthSign> (UniversalId::Type_Birthsign));
    mBirthsigns.addColumn (new NameColumn<ESM::BirthSign>);
    mBirthsigns.addColumn (new TextureColumn<ESM::BirthSign>);
    mBirthsigns.addColumn (new DescriptionColumn<ESM::BirthSign>);

    mSpells.addColumn (new StringIdColumn<ESM::Spell>);
    mSpells.addColumn (new RecordStateColumn<ESM::Spell>);
    mSpells.addColumn (new FixedRecordTypeColumn<ESM::Spell> (UniversalId::Type_Spell));
    mSpells.addColumn (new NameColumn<ESM::Spell>);
    mSpells.addColumn (new SpellTypeColumn<ESM::Spell>);
    mSpells.addColumn (new CostColumn<ESM::Spell>);
    mSpells.addColumn (new FlagColumn<ESM::Spell> (Columns::ColumnId_AutoCalc, 0x1));
    mSpells.addColumn (new FlagColumn<ESM::Spell> (Columns::ColumnId_StarterSpell, 0x2));
    mSpells.addColumn (new FlagColumn<ESM::Spell> (Columns::ColumnId_AlwaysSucceeds, 0x4));

    mTopics.addColumn (new StringIdColumn<ESM::Dialogue>);
    mTopics.addColumn (new RecordStateColumn<ESM::Dialogue>);
    mTopics.addColumn (new FixedRecordTypeColumn<ESM::Dialogue> (UniversalId::Type_Topic));
    mTopics.addColumn (new DialogueTypeColumn<ESM::Dialogue>);

    mJournals.addColumn (new StringIdColumn<ESM::Dialogue>);
    mJournals.addColumn (new RecordStateColumn<ESM::Dialogue>);
    mJournals.addColumn (new FixedRecordTypeColumn<ESM::Dialogue> (UniversalId::Type_Journal));
    mJournals.addColumn (new DialogueTypeColumn<ESM::Dialogue> (true));

    mTopicInfos.addColumn (new StringIdColumn<Info> (true));
    mTopicInfos.addColumn (new RecordStateColumn<Info>);
    mTopicInfos.addColumn (new FixedRecordTypeColumn<Info> (UniversalId::Type_TopicInfo));
    mTopicInfos.addColumn (new TopicColumn<Info> (false));
    mTopicInfos.addColumn (new ActorColumn<Info>);
    mTopicInfos.addColumn (new RaceColumn<Info>);
    mTopicInfos.addColumn (new ClassColumn<Info>);
    mTopicInfos.addColumn (new FactionColumn<Info>);
    mTopicInfos.addColumn (new CellColumn<Info>);
    mTopicInfos.addColumn (new DispositionColumn<Info>);
    mTopicInfos.addColumn (new RankColumn<Info>);
    mTopicInfos.addColumn (new GenderColumn<Info>);
    mTopicInfos.addColumn (new PcFactionColumn<Info>);
    mTopicInfos.addColumn (new PcRankColumn<Info>);
    mTopicInfos.addColumn (new SoundFileColumn<Info>);
    mTopicInfos.addColumn (new ResponseColumn<Info>);

    mJournalInfos.addColumn (new StringIdColumn<Info> (true));
    mJournalInfos.addColumn (new RecordStateColumn<Info>);
    mJournalInfos.addColumn (new FixedRecordTypeColumn<Info> (UniversalId::Type_Journal));
    mJournalInfos.addColumn (new TopicColumn<Info> (true));
    mJournalInfos.addColumn (new QuestStatusTypeColumn<Info>);
    mJournalInfos.addColumn (new QuestIndexColumn<Info>);
    mJournalInfos.addColumn (new QuestDescriptionColumn<Info>);

    mCells.addColumn (new StringIdColumn<Cell>);
    mCells.addColumn (new RecordStateColumn<Cell>);
    mCells.addColumn (new FixedRecordTypeColumn<Cell> (UniversalId::Type_Cell));
    mCells.addColumn (new NameColumn<Cell>);
    mCells.addColumn (new FlagColumn<Cell> (Columns::ColumnId_SleepForbidden, ESM::Cell::NoSleep));
    mCells.addColumn (new FlagColumn<Cell> (Columns::ColumnId_InteriorWater, ESM::Cell::HasWater));
    mCells.addColumn (new FlagColumn<Cell> (Columns::ColumnId_InteriorSky, ESM::Cell::QuasiEx));
    mCells.addColumn (new RegionColumn<Cell>);

    mEnchantments.addColumn (new StringIdColumn<ESM::Enchantment>);
    mEnchantments.addColumn (new RecordStateColumn<ESM::Enchantment>);
    mEnchantments.addColumn (new FixedRecordTypeColumn<ESM::Enchantment> (UniversalId::Type_Enchantment));
    mEnchantments.addColumn (new EnchantmentTypeColumn<ESM::Enchantment>);
    mEnchantments.addColumn (new CostColumn<ESM::Enchantment>);
    mEnchantments.addColumn (new ChargesColumn2<ESM::Enchantment>);
    mEnchantments.addColumn (new AutoCalcColumn<ESM::Enchantment>);

    mBodyParts.addColumn (new StringIdColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new RecordStateColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new FixedRecordTypeColumn<ESM::BodyPart> (UniversalId::Type_BodyPart));
    mBodyParts.addColumn (new BodyPartTypeColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new VampireColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new FlagColumn<ESM::BodyPart> (Columns::ColumnId_Female, ESM::BodyPart::BPF_Female));
    mBodyParts.addColumn (new FlagColumn<ESM::BodyPart> (Columns::ColumnId_Playable, ESM::BodyPart::BPF_NotPlayable, true));
    mBodyParts.addColumn (new MeshTypeColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new ModelColumn<ESM::BodyPart>);
    mBodyParts.addColumn (new RaceColumn<ESM::BodyPart>);

    mRefs.addColumn (new StringIdColumn<CellRef> (true));
    mRefs.addColumn (new RecordStateColumn<CellRef>);
    mRefs.addColumn (new FixedRecordTypeColumn<CellRef> (UniversalId::Type_Reference));
    mRefs.addColumn (new CellColumn<CellRef>);
    mRefs.addColumn (new IdColumn<CellRef>);
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mPos, 0, false));
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mPos, 1, false));
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mPos, 2, false));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mPos, 0, false));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mPos, 1, false));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mPos, 2, false));
    mRefs.addColumn (new ScaleColumn<CellRef>);
    mRefs.addColumn (new OwnerColumn<CellRef>);
    mRefs.addColumn (new SoulColumn<CellRef>);
    mRefs.addColumn (new FactionColumn<CellRef>);
    mRefs.addColumn (new FactionIndexColumn<CellRef>);
    mRefs.addColumn (new ChargesColumn<CellRef>);
    mRefs.addColumn (new EnchantmentChargesColumn<CellRef>);
    mRefs.addColumn (new GoldValueColumn<CellRef>);
    mRefs.addColumn (new TeleportColumn<CellRef>);
    mRefs.addColumn (new TeleportCellColumn<CellRef>);
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mDoorDest, 0, true));
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mDoorDest, 1, true));
    mRefs.addColumn (new PosColumn<CellRef> (&CellRef::mDoorDest, 2, true));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mDoorDest, 0, true));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mDoorDest, 1, true));
    mRefs.addColumn (new RotColumn<CellRef> (&CellRef::mDoorDest, 2, true));
    mRefs.addColumn (new LockLevelColumn<CellRef>);
    mRefs.addColumn (new KeyColumn<CellRef>);
    mRefs.addColumn (new TrapColumn<CellRef>);

    mFilters.addColumn (new StringIdColumn<CSMFilter::Filter>);
    mFilters.addColumn (new RecordStateColumn<CSMFilter::Filter>);
    mFilters.addColumn (new FixedRecordTypeColumn<CSMFilter::Filter> (UniversalId::Type_Filter));
    mFilters.addColumn (new FilterColumn<CSMFilter::Filter>);
    mFilters.addColumn (new DescriptionColumn<CSMFilter::Filter>);
    mFilters.addColumn (new ScopeColumn<CSMFilter::Filter>);

    addModel (new IdTable (&mGlobals), UniversalId::Type_Global);
    addModel (new IdTable (&mGmsts), UniversalId::Type_Gmst);
    addModel (new IdTable (&mSkills), UniversalId::Type_Skill);
    addModel (new IdTable (&mClasses), UniversalId::Type_Class);
    addModel (new IdTable (&mFactions), UniversalId::Type_Faction);
    addModel (new IdTable (&mRaces), UniversalId::Type_Race);
    addModel (new IdTable (&mSounds), UniversalId::Type_Sound);
    addModel (new IdTable (&mScripts), UniversalId::Type_Script);
    addModel (new IdTable (&mRegions), UniversalId::Type_Region);
    addModel (new IdTable (&mBirthsigns), UniversalId::Type_Birthsign);
    addModel (new IdTable (&mSpells), UniversalId::Type_Spell);
    addModel (new IdTable (&mTopics), UniversalId::Type_Topic);
    addModel (new IdTable (&mJournals), UniversalId::Type_Journal);
    addModel (new IdTable (&mTopicInfos, IdTable::Feature_ReorderWithinTopic), UniversalId::Type_TopicInfo);
    addModel (new IdTable (&mJournalInfos, IdTable::Feature_ReorderWithinTopic), UniversalId::Type_JournalInfo);
    addModel (new IdTable (&mCells, IdTable::Feature_ViewId), UniversalId::Type_Cell);
    addModel (new IdTable (&mEnchantments), UniversalId::Type_Enchantment);
    addModel (new IdTable (&mBodyParts), UniversalId::Type_BodyPart);
    addModel (new IdTable (&mReferenceables, IdTable::Feature_Preview),
        UniversalId::Type_Referenceable);
    addModel (new IdTable (&mRefs, IdTable::Feature_ViewCell | IdTable::Feature_Preview), UniversalId::Type_Reference);
    addModel (new IdTable (&mFilters), UniversalId::Type_Filter);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_Mesh)),
        UniversalId::Type_Mesh);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_Icon)),
        UniversalId::Type_Icon);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_Music)),
        UniversalId::Type_Music);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_SoundRes)),
        UniversalId::Type_SoundRes);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_Texture)),
        UniversalId::Type_Texture);
    addModel (new ResourceTable (&mResourcesManager.get (UniversalId::Type_Video)),
        UniversalId::Type_Video);
}

CSMWorld::Data::~Data()
{
    for (std::vector<QAbstractItemModel *>::iterator iter (mModels.begin()); iter!=mModels.end(); ++iter)
        delete *iter;

    delete mReader;
}

const CSMWorld::IdCollection<ESM::Global>& CSMWorld::Data::getGlobals() const
{
    return mGlobals;
}

CSMWorld::IdCollection<ESM::Global>& CSMWorld::Data::getGlobals()
{
    return mGlobals;
}

const CSMWorld::IdCollection<ESM::GameSetting>& CSMWorld::Data::getGmsts() const
{
    return mGmsts;
}

CSMWorld::IdCollection<ESM::GameSetting>& CSMWorld::Data::getGmsts()
{
    return mGmsts;
}

const CSMWorld::IdCollection<ESM::Skill>& CSMWorld::Data::getSkills() const
{
    return mSkills;
}

CSMWorld::IdCollection<ESM::Skill>& CSMWorld::Data::getSkills()
{
    return mSkills;
}

const CSMWorld::IdCollection<ESM::Class>& CSMWorld::Data::getClasses() const
{
    return mClasses;
}

CSMWorld::IdCollection<ESM::Class>& CSMWorld::Data::getClasses()
{
    return mClasses;
}

const CSMWorld::IdCollection<ESM::Faction>& CSMWorld::Data::getFactions() const
{
    return mFactions;
}

CSMWorld::IdCollection<ESM::Faction>& CSMWorld::Data::getFactions()
{
    return mFactions;
}

const CSMWorld::IdCollection<ESM::Race>& CSMWorld::Data::getRaces() const
{
    return mRaces;
}

CSMWorld::IdCollection<ESM::Race>& CSMWorld::Data::getRaces()
{
    return mRaces;
}

const CSMWorld::IdCollection<ESM::Sound>& CSMWorld::Data::getSounds() const
{
    return mSounds;
}

CSMWorld::IdCollection<ESM::Sound>& CSMWorld::Data::getSounds()
{
    return mSounds;
}

const CSMWorld::IdCollection<ESM::Script>& CSMWorld::Data::getScripts() const
{
    return mScripts;
}

CSMWorld::IdCollection<ESM::Script>& CSMWorld::Data::getScripts()
{
    return mScripts;
}

const CSMWorld::IdCollection<ESM::Region>& CSMWorld::Data::getRegions() const
{
    return mRegions;
}

CSMWorld::IdCollection<ESM::Region>& CSMWorld::Data::getRegions()
{
    return mRegions;
}

const CSMWorld::IdCollection<ESM::BirthSign>& CSMWorld::Data::getBirthsigns() const
{
    return mBirthsigns;
}

CSMWorld::IdCollection<ESM::BirthSign>& CSMWorld::Data::getBirthsigns()
{
    return mBirthsigns;
}

const CSMWorld::IdCollection<ESM::Spell>& CSMWorld::Data::getSpells() const
{
    return mSpells;
}

CSMWorld::IdCollection<ESM::Spell>& CSMWorld::Data::getSpells()
{
    return mSpells;
}


const CSMWorld::IdCollection<ESM::Dialogue>& CSMWorld::Data::getTopics() const
{
    return mTopics;
}

CSMWorld::IdCollection<ESM::Dialogue>& CSMWorld::Data::getTopics()
{
    return mTopics;
}

const CSMWorld::IdCollection<ESM::Dialogue>& CSMWorld::Data::getJournals() const
{
    return mJournals;
}

CSMWorld::IdCollection<ESM::Dialogue>& CSMWorld::Data::getJournals()
{
    return mJournals;
}

const CSMWorld::InfoCollection& CSMWorld::Data::getTopicInfos() const
{
    return mTopicInfos;
}

CSMWorld::InfoCollection& CSMWorld::Data::getTopicInfos()
{
    return mTopicInfos;
}

const CSMWorld::InfoCollection& CSMWorld::Data::getJournalInfos() const
{
    return mJournalInfos;
}

CSMWorld::InfoCollection& CSMWorld::Data::getJournalInfos()
{
    return mJournalInfos;
}

const CSMWorld::IdCollection<CSMWorld::Cell>& CSMWorld::Data::getCells() const
{
    return mCells;
}

CSMWorld::IdCollection<CSMWorld::Cell>& CSMWorld::Data::getCells()
{
    return mCells;
}

const CSMWorld::RefIdCollection& CSMWorld::Data::getReferenceables() const
{
    return mReferenceables;
}

CSMWorld::RefIdCollection& CSMWorld::Data::getReferenceables()
{
    return mReferenceables;
}

const CSMWorld::RefCollection& CSMWorld::Data::getReferences() const
{
    return mRefs;
}

CSMWorld::RefCollection& CSMWorld::Data::getReferences()
{
    return mRefs;
}

const CSMWorld::IdCollection<CSMFilter::Filter>& CSMWorld::Data::getFilters() const
{
    return mFilters;
}

CSMWorld::IdCollection<CSMFilter::Filter>& CSMWorld::Data::getFilters()
{
    return mFilters;
}

const CSMWorld::IdCollection<ESM::Enchantment>& CSMWorld::Data::getEnchantments() const
{
    return mEnchantments;
}

CSMWorld::IdCollection<ESM::Enchantment>& CSMWorld::Data::getEnchantments()
{
    return mEnchantments;
}

const CSMWorld::IdCollection<ESM::BodyPart>& CSMWorld::Data::getBodyParts() const
{
    return mBodyParts;
}

CSMWorld::IdCollection<ESM::BodyPart>& CSMWorld::Data::getBodyParts()
{
    return mBodyParts;
}

const CSMWorld::Resources& CSMWorld::Data::getResources (const UniversalId& id) const
{
    return mResourcesManager.get (UniversalId::getParentType (id.getType()));
}

QAbstractItemModel *CSMWorld::Data::getTableModel (const CSMWorld::UniversalId& id)
{
    std::map<UniversalId::Type, QAbstractItemModel *>::iterator iter = mModelIndex.find (id.getType());

    if (iter==mModelIndex.end())
    {
        // try creating missing (secondary) tables on the fly
        //
        // Note: We create these tables here so we don't have to deal with them during load/initial
        // construction of the ESX data where no update signals are available.
        if (id.getType()==UniversalId::Type_RegionMap)
        {
            RegionMap *table = 0;
            addModel (table = new RegionMap (*this), UniversalId::Type_RegionMap, false);
            return table;
        }
        throw std::logic_error ("No table model available for " + id.toString());
    }

    return iter->second;
}

void CSMWorld::Data::merge()
{
    mGlobals.merge();
}

int CSMWorld::Data::startLoading (const boost::filesystem::path& path, bool base, bool project)
{
    delete mReader;
    mReader = 0;
    mDialogue = 0;
    mRefLoadCache.clear();

    mReader = new ESM::ESMReader;
    mReader->setEncoder (&mEncoder);
    mReader->open (path.string());

    mBase = base;
    mProject = project;

    mAuthor = mReader->getAuthor();
    mDescription = mReader->getDesc();

    return mReader->getRecordCount();
}

bool CSMWorld::Data::continueLoading (CSMDoc::Stage::Messages& messages)
{
    if (!mReader)
        throw std::logic_error ("can't continue loading, because no load has been started");

    if (!mReader->hasMoreRecs())
    {
        delete mReader;
        mReader = 0;
        mDialogue = 0;
        mRefLoadCache.clear();
        return true;
    }

    ESM::NAME n = mReader->getRecName();
    mReader->getRecHeader();

    switch (n.val)
    {
        case ESM::REC_GLOB: mGlobals.load (*mReader, mBase); break;
        case ESM::REC_GMST: mGmsts.load (*mReader, mBase); break;
        case ESM::REC_SKIL: mSkills.load (*mReader, mBase); break;
        case ESM::REC_CLAS: mClasses.load (*mReader, mBase); break;
        case ESM::REC_FACT: mFactions.load (*mReader, mBase); break;
        case ESM::REC_RACE: mRaces.load (*mReader, mBase); break;
        case ESM::REC_SOUN: mSounds.load (*mReader, mBase); break;
        case ESM::REC_SCPT: mScripts.load (*mReader, mBase); break;
        case ESM::REC_REGN: mRegions.load (*mReader, mBase); break;
        case ESM::REC_BSGN: mBirthsigns.load (*mReader, mBase); break;
        case ESM::REC_SPEL: mSpells.load (*mReader, mBase); break;
        case ESM::REC_ENCH: mEnchantments.load (*mReader, mBase); break;
        case ESM::REC_BODY: mBodyParts.load (*mReader, mBase); break;

        case ESM::REC_CELL:
        {
            mCells.load (*mReader, mBase);
            std::string cellId = Misc::StringUtils::lowerCase (mCells.getId (mCells.getSize()-1));
            mRefs.load (*mReader, mCells.getSize()-1, mBase, mRefLoadCache[cellId], messages);
            break;
        }

        case ESM::REC_ACTI: mReferenceables.load (*mReader, mBase, UniversalId::Type_Activator); break;
        case ESM::REC_ALCH: mReferenceables.load (*mReader, mBase, UniversalId::Type_Potion); break;
        case ESM::REC_APPA: mReferenceables.load (*mReader, mBase, UniversalId::Type_Apparatus); break;
        case ESM::REC_ARMO: mReferenceables.load (*mReader, mBase, UniversalId::Type_Armor); break;
        case ESM::REC_BOOK: mReferenceables.load (*mReader, mBase, UniversalId::Type_Book); break;
        case ESM::REC_CLOT: mReferenceables.load (*mReader, mBase, UniversalId::Type_Clothing); break;
        case ESM::REC_CONT: mReferenceables.load (*mReader, mBase, UniversalId::Type_Container); break;
        case ESM::REC_CREA: mReferenceables.load (*mReader, mBase, UniversalId::Type_Creature); break;
        case ESM::REC_DOOR: mReferenceables.load (*mReader, mBase, UniversalId::Type_Door); break;
        case ESM::REC_INGR: mReferenceables.load (*mReader, mBase, UniversalId::Type_Ingredient); break;
        case ESM::REC_LEVC:
            mReferenceables.load (*mReader, mBase, UniversalId::Type_CreatureLevelledList); break;
        case ESM::REC_LEVI:
            mReferenceables.load (*mReader, mBase, UniversalId::Type_ItemLevelledList); break;
        case ESM::REC_LIGH: mReferenceables.load (*mReader, mBase, UniversalId::Type_Light); break;
        case ESM::REC_LOCK: mReferenceables.load (*mReader, mBase, UniversalId::Type_Lockpick); break;
        case ESM::REC_MISC:
            mReferenceables.load (*mReader, mBase, UniversalId::Type_Miscellaneous); break;
        case ESM::REC_NPC_: mReferenceables.load (*mReader, mBase, UniversalId::Type_Npc); break;
        case ESM::REC_PROB: mReferenceables.load (*mReader, mBase, UniversalId::Type_Probe); break;
        case ESM::REC_REPA: mReferenceables.load (*mReader, mBase, UniversalId::Type_Repair); break;
        case ESM::REC_STAT: mReferenceables.load (*mReader, mBase, UniversalId::Type_Static); break;
        case ESM::REC_WEAP: mReferenceables.load (*mReader, mBase, UniversalId::Type_Weapon); break;

        case ESM::REC_DIAL:
        {
            std::string id = mReader->getHNOString ("NAME");

            ESM::Dialogue record;
            record.mId = id;
            record.load (*mReader);

            if (record.mType==ESM::Dialogue::Journal)
            {
                mJournals.load (record, mBase);
                mDialogue = &mJournals.getRecord (id).get();
            }
            else if (record.mType==ESM::Dialogue::Deleted)
            {
                mDialogue = 0; // record vector can be shuffled around which would make pointer
                               // to record invalid

                if (mJournals.tryDelete (id))
                {
                    /// \todo handle info records
                }
                else if (mTopics.tryDelete (id))
                {
                    /// \todo handle info records
                }
                else
                {
                    messages.push_back (std::make_pair (UniversalId::Type_None,
                        "Trying to delete dialogue record " + id + " which does not exist"));
                }
            }
            else
            {
                mTopics.load (record, mBase);
                mDialogue = &mTopics.getRecord (id).get();
            }

            break;
        }

        case ESM::REC_INFO:
        {
            if (!mDialogue)
            {
                messages.push_back (std::make_pair (UniversalId::Type_None,
                    "Found info record not following a dialogue record"));

                mReader->skipRecord();
                break;
            }

            if (mDialogue->mType==ESM::Dialogue::Journal)
                mJournalInfos.load (*mReader, mBase, *mDialogue);
            else
                mTopicInfos.load (*mReader, mBase, *mDialogue);

            break;
        }

        case ESM::REC_FILT:

            if (mProject)
            {
                mFilters.load (*mReader, mBase);
                mFilters.setData (mFilters.getSize()-1,
                    mFilters.findColumnIndex (CSMWorld::Columns::ColumnId_Scope),
                    static_cast<int> (CSMFilter::Filter::Scope_Project));
                break;
            }

            // fall through (filter record in a content file is an error with format 0)

        default:

            messages.push_back (std::make_pair (UniversalId::Type_None,
                "Unsupported record type: " + n.toString()));

            mReader->skipRecord();
    }

    return false;
}

bool CSMWorld::Data::hasId (const std::string& id) const
{
    return
        getGlobals().searchId (id)!=-1 ||
        getGmsts().searchId (id)!=-1 ||
        getSkills().searchId (id)!=-1 ||
        getClasses().searchId (id)!=-1 ||
        getFactions().searchId (id)!=-1 ||
        getRaces().searchId (id)!=-1 ||
        getSounds().searchId (id)!=-1 ||
        getScripts().searchId (id)!=-1 ||
        getRegions().searchId (id)!=-1 ||
        getBirthsigns().searchId (id)!=-1 ||
        getSpells().searchId (id)!=-1 ||
        getTopics().searchId (id)!=-1 ||
        getJournals().searchId (id)!=-1 ||
        getCells().searchId (id)!=-1 ||
        getEnchantments().searchId (id)!=-1 ||
        getBodyParts().searchId (id)!=-1 ||
        getReferenceables().searchId (id)!=-1;
}

int CSMWorld::Data::count (RecordBase::State state) const
{
    return
        count (state, mGlobals) +
        count (state, mGmsts) +
        count (state, mSkills) +
        count (state, mClasses) +
        count (state, mFactions) +
        count (state, mRaces) +
        count (state, mSounds) +
        count (state, mScripts) +
        count (state, mRegions) +
        count (state, mBirthsigns) +
        count (state, mSpells) +
        count (state, mCells) +
        count (state, mEnchantments) +
        count (state, mBodyParts) +
        count (state, mReferenceables);
}

void CSMWorld::Data::setDescription (const std::string& description)
{
    mDescription = description;
}

std::string CSMWorld::Data::getDescription() const
{
    return mDescription;
}

void CSMWorld::Data::setAuthor (const std::string& author)
{
    mAuthor = author;
}

std::string CSMWorld::Data::getAuthor() const
{
    return mAuthor;
}

std::vector<std::string> CSMWorld::Data::getIds (bool listDeleted) const
{
    std::vector<std::string> ids;

    appendIds (ids, mGlobals, listDeleted);
    appendIds (ids, mGmsts, listDeleted);
    appendIds (ids, mClasses, listDeleted);
    appendIds (ids, mFactions, listDeleted);
    appendIds (ids, mRaces, listDeleted);
    appendIds (ids, mSounds, listDeleted);
    appendIds (ids, mScripts, listDeleted);
    appendIds (ids, mRegions, listDeleted);
    appendIds (ids, mBirthsigns, listDeleted);
    appendIds (ids, mSpells, listDeleted);
    appendIds (ids, mTopics, listDeleted);
    appendIds (ids, mJournals, listDeleted);
    appendIds (ids, mCells, listDeleted);
    appendIds (ids, mEnchantments, listDeleted);
    appendIds (ids, mBodyParts, listDeleted);
    appendIds (ids, mReferenceables, listDeleted);

    std::sort (ids.begin(), ids.end());

    return ids;
}

void CSMWorld::Data::dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (topLeft.column()<=0)
        emit idListChanged();
}

void CSMWorld::Data::rowsChanged (const QModelIndex& parent, int start, int end)
{
    emit idListChanged();
}
