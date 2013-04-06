
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
    mGmsts.addColumn (new VarTypeColumn<ESM::GameSetting> (ColumnBase::Display_GmstVarType));
    mGmsts.addColumn (new VarValueColumn<ESM::GameSetting>);

    mSkills.addColumn (new StringIdColumn<ESM::Skill>);
    mSkills.addColumn (new RecordStateColumn<ESM::Skill>);
    mSkills.addColumn (new AttributeColumn<ESM::Skill>);
    mSkills.addColumn (new SpecialisationColumn<ESM::Skill>);
    for (int i=0; i<4; ++i)
        mSkills.addColumn (new UseValueColumn<ESM::Skill> (i));
    mSkills.addColumn (new DescriptionColumn<ESM::Skill>);

    mClasses.addColumn (new StringIdColumn<ESM::Class>);
    mClasses.addColumn (new RecordStateColumn<ESM::Class>);
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
    mFactions.addColumn (new NameColumn<ESM::Faction>);
    mFactions.addColumn (new AttributesColumn<ESM::Faction> (0));
    mFactions.addColumn (new AttributesColumn<ESM::Faction> (1));
    mFactions.addColumn (new HiddenColumn<ESM::Faction>);
    for (int i=0; i<6; ++i)
        mFactions.addColumn (new SkillsColumn<ESM::Faction> (i));

    mRaces.addColumn (new StringIdColumn<ESM::Race>);
    mRaces.addColumn (new RecordStateColumn<ESM::Race>);
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
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_Volume));
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_MinRange));
    mSounds.addColumn (new SoundParamColumn<ESM::Sound> (SoundParamColumn<ESM::Sound>::Type_MaxRange));
    mSounds.addColumn (new SoundFileColumn<ESM::Sound>);

    addModel (new IdTable (&mGlobals), UniversalId::Type_Globals, UniversalId::Type_Global);
    addModel (new IdTable (&mGmsts), UniversalId::Type_Gmsts, UniversalId::Type_Gmst);
    addModel (new IdTable (&mSkills), UniversalId::Type_Skills, UniversalId::Type_Skill);
    addModel (new IdTable (&mClasses), UniversalId::Type_Classes, UniversalId::Type_Class);
    addModel (new IdTable (&mFactions), UniversalId::Type_Factions, UniversalId::Type_Faction);
    addModel (new IdTable (&mRaces), UniversalId::Type_Races, UniversalId::Type_Race);
    addModel (new IdTable (&mSounds), UniversalId::Type_Sounds, UniversalId::Type_Sound);
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

            default:

                /// \todo throw an exception instead, once all records are implemented
                reader.skipRecord();
        }
    }
}