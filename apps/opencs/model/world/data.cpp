
#include "data.hpp"

#include <stdexcept>

#include <QAbstractItemModel>

#include <components/esm/esmreader.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/loadglob.hpp>

#include "idtable.hpp"
#include "columns.hpp"

void CSMWorld::Data::addModel (QAbstractItemModel *model, UniversalId::Type type1,
    UniversalId::Type type2)
{
    mModels.push_back (model);
    mModelIndex.insert (std::make_pair (type1, model));

    if (type2!=UniversalId::Type_None)
        mModelIndex.insert (std::make_pair (type2, model));
}

CSMWorld::Data::Data()
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
    for (int i=0; i<6; ++i)
        mFactions.addColumn (new SkillsColumn<ESM::Faction> (i));

    mRaces.addColumn (new StringIdColumn<ESM::Race>);
    mRaces.addColumn (new RecordStateColumn<ESM::Race>);
    mRaces.addColumn (new FixedRecordTypeColumn<ESM::Race> (UniversalId::Type_Race));
    mRaces.addColumn (new NameColumn<ESM::Race>);
    mRaces.addColumn (new DescriptionColumn<ESM::Race>);
    mRaces.addColumn (new FlagColumn<ESM::Race> ("Playable", 0x1));
    mRaces.addColumn (new FlagColumn<ESM::Race> ("Beast Race", 0x2));
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
    mSpells.addColumn (new FlagColumn<ESM::Spell> ("Autocalc", 0x1));
    mSpells.addColumn (new FlagColumn<ESM::Spell> ("Starter Spell", 0x2));
    mSpells.addColumn (new FlagColumn<ESM::Spell> ("Always Succeeds", 0x4));

    mCells.addColumn (new StringIdColumn<Cell>);
    mCells.addColumn (new RecordStateColumn<Cell>);
    mCells.addColumn (new FixedRecordTypeColumn<Cell> (UniversalId::Type_Cell));
    mCells.addColumn (new NameColumn<Cell>);
    mCells.addColumn (new FlagColumn<Cell> ("Sleep forbidden", ESM::Cell::NoSleep));
    mCells.addColumn (new FlagColumn<Cell> ("Interior Water", ESM::Cell::HasWater));
    mCells.addColumn (new FlagColumn<Cell> ("Interior Sky", ESM::Cell::QuasiEx));
    mCells.addColumn (new RegionColumn<Cell>);

    addModel (new IdTable (&mGlobals), UniversalId::Type_Globals, UniversalId::Type_Global);
    addModel (new IdTable (&mGmsts), UniversalId::Type_Gmsts, UniversalId::Type_Gmst);
    addModel (new IdTable (&mSkills), UniversalId::Type_Skills, UniversalId::Type_Skill);
    addModel (new IdTable (&mClasses), UniversalId::Type_Classes, UniversalId::Type_Class);
    addModel (new IdTable (&mFactions), UniversalId::Type_Factions, UniversalId::Type_Faction);
    addModel (new IdTable (&mRaces), UniversalId::Type_Races, UniversalId::Type_Race);
    addModel (new IdTable (&mSounds), UniversalId::Type_Sounds, UniversalId::Type_Sound);
    addModel (new IdTable (&mScripts), UniversalId::Type_Scripts, UniversalId::Type_Script);
    addModel (new IdTable (&mRegions), UniversalId::Type_Regions, UniversalId::Type_Region);
    addModel (new IdTable (&mBirthsigns), UniversalId::Type_Birthsigns, UniversalId::Type_Birthsign);
    addModel (new IdTable (&mSpells), UniversalId::Type_Spells, UniversalId::Type_Spell);
    addModel (new IdTable (&mCells), UniversalId::Type_Cells, UniversalId::Type_Cell);
    addModel (new IdTable (&mReferenceables), UniversalId::Type_Referenceables,
        UniversalId::Type_Referenceable);
}

CSMWorld::Data::~Data()
{
    for (std::vector<QAbstractItemModel *>::iterator iter (mModels.begin()); iter!=mModels.end(); ++iter)
        delete *iter;
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

QAbstractItemModel *CSMWorld::Data::getTableModel (const UniversalId& id)
{
    std::map<UniversalId::Type, QAbstractItemModel *>::iterator iter = mModelIndex.find (id.getType());

    if (iter==mModelIndex.end())
        throw std::logic_error ("No table model available for " + id.toString());

    return iter->second;
}

void CSMWorld::Data::merge()
{
    mGlobals.merge();
}

void CSMWorld::Data::loadFile (const boost::filesystem::path& path, bool base)
{
    ESM::ESMReader reader;

    /// \todo set encoding properly, once config implementation has been fixed.
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding ("win1252"));
    reader.setEncoder (&encoder);

    reader.open (path.string());

    // Note: We do not need to send update signals here, because at this point the model is not connected
    // to any view.
    while (reader.hasMoreRecs())
    {
        ESM::NAME n = reader.getRecName();
        reader.getRecHeader();

        switch (n.val)
        {
            case ESM::REC_GLOB: mGlobals.load (reader, base); break;
            case ESM::REC_GMST: mGmsts.load (reader, base); break;
            case ESM::REC_SKIL: mSkills.load (reader, base); break;
            case ESM::REC_CLAS: mClasses.load (reader, base); break;
            case ESM::REC_FACT: mFactions.load (reader, base); break;
            case ESM::REC_RACE: mRaces.load (reader, base); break;
            case ESM::REC_SOUN: mSounds.load (reader, base); break;
            case ESM::REC_SCPT: mScripts.load (reader, base); break;
            case ESM::REC_REGN: mRegions.load (reader, base); break;
            case ESM::REC_BSGN: mBirthsigns.load (reader, base); break;
            case ESM::REC_SPEL: mSpells.load (reader, base); break;
            case ESM::REC_CELL: mCells.load (reader, base); break;

            case ESM::REC_ACTI: mReferenceables.load (reader, base, UniversalId::Type_Activator); break;
            case ESM::REC_ALCH: mReferenceables.load (reader, base, UniversalId::Type_Potion); break;
            case ESM::REC_APPA: mReferenceables.load (reader, base, UniversalId::Type_Apparatus); break;
            case ESM::REC_ARMO: mReferenceables.load (reader, base, UniversalId::Type_Armor); break;
            case ESM::REC_BOOK: mReferenceables.load (reader, base, UniversalId::Type_Book); break;
            case ESM::REC_CLOT: mReferenceables.load (reader, base, UniversalId::Type_Clothing); break;
            case ESM::REC_CONT: mReferenceables.load (reader, base, UniversalId::Type_Container); break;
            case ESM::REC_CREA: mReferenceables.load (reader, base, UniversalId::Type_Creature); break;
            case ESM::REC_DOOR: mReferenceables.load (reader, base, UniversalId::Type_Door); break;
            case ESM::REC_INGR: mReferenceables.load (reader, base, UniversalId::Type_Ingredient); break;
            case ESM::REC_LEVC:
                mReferenceables.load (reader, base, UniversalId::Type_CreatureLevelledList); break;
            case ESM::REC_LEVI:
                mReferenceables.load (reader, base, UniversalId::Type_ItemLevelledList); break;
            case ESM::REC_LIGH: mReferenceables.load (reader, base, UniversalId::Type_Light); break;
            case ESM::REC_LOCK: mReferenceables.load (reader, base, UniversalId::Type_Lockpick); break;
            case ESM::REC_MISC:
                mReferenceables.load (reader, base, UniversalId::Type_Miscellaneous); break;
            case ESM::REC_NPC_: mReferenceables.load (reader, base, UniversalId::Type_Npc); break;
            case ESM::REC_PROB: mReferenceables.load (reader, base, UniversalId::Type_Probe); break;
            case ESM::REC_REPA: mReferenceables.load (reader, base, UniversalId::Type_Repair); break;
            case ESM::REC_STAT: mReferenceables.load (reader, base, UniversalId::Type_Static); break;
            case ESM::REC_WEAP: mReferenceables.load (reader, base, UniversalId::Type_Weapon); break;

            default:

                /// \todo throw an exception instead, once all records are implemented
                reader.skipRecord();
        }
    }
}