#include "npcautocalc.hpp"

#include <QStringList>

#include <components/autocalc/autocalc.hpp>
#include <components/autocalc/autocalcspell.hpp>
#include <components/autocalc/store.hpp>

#include "npcstats.hpp"
#include "data.hpp"
#include "idtable.hpp"
#include "idtree.hpp"

namespace
{
    class CSStore : public AutoCalc::StoreCommon
    {
        const CSMWorld::IdCollection<ESM::GameSetting>& mGmstTable;
        const CSMWorld::IdCollection<ESM::Skill>& mSkillTable;
        const CSMWorld::IdCollection<ESM::MagicEffect>& mMagicEffectTable;
        const CSMWorld::NestedIdCollection<ESM::Spell>& mSpells;

    public:

        CSStore(const CSMWorld::IdCollection<ESM::GameSetting>& gmst,
                const CSMWorld::IdCollection<ESM::Skill>& skills,
                const CSMWorld::IdCollection<ESM::MagicEffect>& magicEffects,
                const CSMWorld::NestedIdCollection<ESM::Spell>& spells)
            : mGmstTable(gmst), mSkillTable(skills), mMagicEffectTable(magicEffects), mSpells(spells)
        {}

        ~CSStore() {}

        virtual int findGmstInt(const std::string& name) const
        {
            return mGmstTable.getRecord(name).get().getInt();
        }

        virtual float findGmstFloat(const std::string& name) const
        {
            return mGmstTable.getRecord(name).get().getFloat();
        }

        virtual const ESM::Skill *findSkill(int index) const
        {
            // if the skill does not exist, throws std::runtime_error ("invalid ID: " + id)
            return &mSkillTable.getRecord(ESM::Skill::indexToId(index)).get();
        }

        virtual const ESM::MagicEffect* findMagicEffect(int id) const
        {
            // if the magic effect does not exist, throws std::runtime_error ("invalid ID: " + id)
            return &mMagicEffectTable.getRecord(ESM::MagicEffect::indexToId((short)id)).get();
        }

        virtual void getSpells(std::vector<ESM::Spell*>& spells)
        {
            // prepare data in a format used by OpenMW store
            for (int index = 0; index < mSpells.getSize(); ++index)
                spells.push_back(const_cast<ESM::Spell *>(&mSpells.getRecord(index).get()));
        }
    };

    unsigned short autoCalculateMana(const AutoCalc::StatsBase& stats)
    {
        return stats.getBaseAttribute(ESM::Attribute::Intelligence) * 2;
    }

    unsigned short autoCalculateFatigue(const AutoCalc::StatsBase& stats)
    {
        return stats.getBaseAttribute(ESM::Attribute::Strength)
                + stats.getBaseAttribute(ESM::Attribute::Willpower)
                + stats.getBaseAttribute(ESM::Attribute::Agility)
                + stats.getBaseAttribute(ESM::Attribute::Endurance);
    }
}

CSMWorld::NpcAutoCalc::NpcAutoCalc (const Data& data,
    const IdTable *gmsts, const IdTable *skills, const IdTable *classes, const IdTree *races, const IdTree *objects)
: mData(data), mSkillModel(skills), mClassModel(classes), mRaceModel(races)
{
    // for autocalc updates when gmst/race/class/skils tables change
    connect (gmsts, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (gmstDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mSkillModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (skillDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mClassModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (classDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mRaceModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (raceDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (objects, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (npcDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (this, SIGNAL (updateNpcAutocalc (int, const std::string&)),
            objects, SLOT (updateNpcAutocalc (int, const std::string&)));

    //connect (this, SIGNAL (cacheNpcStats (const std::string&, NpcStats*)),
            //this, SLOT (cacheNpcStatsEvent (const std::string&, NpcStats*)));
}

CSMWorld::NpcAutoCalc::~NpcAutoCalc()
{
    clearNpcStatsCache();
}

void CSMWorld::NpcAutoCalc::skillDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // mData.mAttribute (affects attributes skill bonus autocalc)
    // mData.mSpecialization (affects skills autocalc)
    int attributeColumn = mSkillModel->findColumnIndex(CSMWorld::Columns::ColumnId_Attribute);
    int specialisationColumn = mSkillModel->findColumnIndex(CSMWorld::Columns::ColumnId_Specialisation);

    if ((topLeft.column() <= attributeColumn && attributeColumn <= bottomRight.column())
        || (topLeft.column() <= specialisationColumn && specialisationColumn <= bottomRight.column()))
    {
        clearNpcStatsCache();

        std::string empty;
        emit updateNpcAutocalc(0/*all*/, empty);
    }
}

void CSMWorld::NpcAutoCalc::classDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // update autocalculated attributes/skills of every NPC with matching class
    // - mData.mAttribute[2]
    // - mData.mSkills[5][2]
    // - mData.mSpecialization
    int attribute1Column = mClassModel->findColumnIndex(CSMWorld::Columns::ColumnId_Attribute1);   // +1
    int majorSkill1Column = mClassModel->findColumnIndex(CSMWorld::Columns::ColumnId_MajorSkill1); // +4
    int minorSkill1Column = mClassModel->findColumnIndex(CSMWorld::Columns::ColumnId_MinorSkill1); // +4
    int specialisationColumn = mClassModel->findColumnIndex(CSMWorld::Columns::ColumnId_Specialisation);

    if ((topLeft.column() > attribute1Column+1 || attribute1Column > bottomRight.column())
        && (topLeft.column() > majorSkill1Column+4 || majorSkill1Column > bottomRight.column())
        && (topLeft.column() > minorSkill1Column+4 || minorSkill1Column > bottomRight.column())
        && (topLeft.column() > specialisationColumn || specialisationColumn > bottomRight.column()))
    {
        return;
    }

    // get the affected class
    int idColumn = mClassModel->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    for (int classRow = topLeft.row(); classRow <= bottomRight.row(); ++classRow)
    {
        clearNpcStatsCache();

        std::string classId =
            mClassModel->data(mClassModel->index(classRow, idColumn)).toString().toUtf8().constData();
        emit updateNpcAutocalc(1/*class*/, classId);
    }
}

void CSMWorld::NpcAutoCalc::raceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // affects racial bonus attributes & skills
    // - mData.mAttributeValues[]
    // - mData.mBonus[].mBonus
    // - mPowers.mList[]
    int attrColumn = mRaceModel->findColumnIndex(CSMWorld::Columns::ColumnId_RaceAttributes);
    int bonusColumn = mRaceModel->findColumnIndex(CSMWorld::Columns::ColumnId_RaceSkillBonus);
    int powersColumn = mRaceModel->findColumnIndex(CSMWorld::Columns::ColumnId_PowerList);

    bool match = false;
    int raceRow = topLeft.row();
    int raceEnd = bottomRight.row();
    if (topLeft.parent().isValid() && bottomRight.parent().isValid())
    {
        if ((topLeft.parent().column() <= attrColumn && attrColumn <= bottomRight.parent().column())
            || (topLeft.parent().column() <= bonusColumn && bonusColumn <= bottomRight.parent().column())
            || (topLeft.parent().column() <= powersColumn && powersColumn <= bottomRight.parent().column()))
        {
            match = true; // TODO: check for specific nested column?
            raceRow = topLeft.parent().row();
            raceEnd = bottomRight.parent().row();
        }
    }
    else
    {
        if ((topLeft.column() <= attrColumn && attrColumn <= bottomRight.column())
            || (topLeft.column() <= bonusColumn && bonusColumn <= bottomRight.column())
            || (topLeft.column() <= powersColumn && powersColumn <= bottomRight.column()))
        {
            match = true; // maybe the whole table changed
        }
    }

    if (!match)
        return;

    // update autocalculated attributes/skills of every NPC with matching race
    int idColumn = mRaceModel->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    for (; raceRow <= raceEnd; ++raceRow)
    {
        clearNpcStatsCache();

        std::string raceId =
            mRaceModel->data(mRaceModel->index(raceRow, idColumn)).toString().toUtf8().constData();
        emit updateNpcAutocalc(2/*race*/, raceId);
    }
}

void CSMWorld::NpcAutoCalc::npcDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // TODO: for now always recalculate
    clearNpcStatsCache();

    // TODO: check if below signal slows things down
    std::string empty;
    emit updateNpcAutocalc(0/*all*/, empty);
}

void CSMWorld::NpcAutoCalc::gmstDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    static const QStringList gmsts(QStringList()<< "fNPCbaseMagickaMult" << "fAutoSpellChance"
            << "fEffectCostMult" << "iAutoSpellAlterationMax" << "iAutoSpellConjurationMax"
            << "iAutoSpellDestructionMax" << "iAutoSpellIllusionMax" << "iAutoSpellMysticismMax"
            << "iAutoSpellRestorationMax" << "iAutoSpellTimesCanCast" << "iAutoSpellAttSkillMin");

    bool match = false;
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
    {
        if (gmsts.contains(mData.getGmsts().getRecord(row).get().mId.c_str()))
        {
            match = true;
            break;
        }
    }

    if (!match)
        return;

    clearNpcStatsCache();

    std::string empty;
    emit updateNpcAutocalc(0/*all*/, empty);
}

void CSMWorld::NpcAutoCalc::clearNpcStatsCache ()
{
    for (std::map<const std::string, CSMWorld::NpcStats*>::iterator it (mNpcStatCache.begin());
            it != mNpcStatCache.end(); ++it)
        delete it->second;

    mNpcStatCache.clear();
}

CSMWorld::NpcStats* CSMWorld::NpcAutoCalc::npcAutoCalculate(const ESM::NPC& npc) const
{
    CSMWorld::NpcStats *cachedStats = getCachedNpcData (npc.mId);
    if (cachedStats)
        return cachedStats;

    int raceIndex = mData.getRaces().searchId(npc.mRace);
    int classIndex = mData.getClasses().searchId(npc.mClass);
    // this can happen when creating a new game from scratch
    if (raceIndex == -1 || classIndex == -1)
        return 0;

    const ESM::Race *race = &mData.getRaces().getRecord(raceIndex).get();
    const ESM::Class *class_ = &mData.getClasses().getRecord(classIndex).get();

    bool autoCalc = npc.mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS;
    short level = npc.mNpdt52.mLevel;
    if (autoCalc)
        level = npc.mNpdt12.mLevel;

    std::auto_ptr<CSMWorld::NpcStats> stats (new CSMWorld::NpcStats());

    CSStore store(mData.getGmsts(), mData.getSkills(), mData.getMagicEffects(), static_cast<const CSMWorld::NestedIdCollection<ESM::Spell>&>(mData.getSpells()));

    if (autoCalc)
    {
        AutoCalc::autoCalcAttributesImpl (&npc, race, class_, level, *stats, &store);

        stats->setHealth(autoCalculateHealth(level, class_, *stats));
        stats->setMana(autoCalculateMana(*stats));
        stats->setFatigue(autoCalculateFatigue(*stats));

        AutoCalc::autoCalcSkillsImpl(&npc, race, class_, level, *stats, &store);

        AutoCalc::autoCalculateSpells(race, *stats, &store);
    }
    else
    {
        for (std::vector<std::string>::const_iterator it = npc.mSpells.mList.begin();
                it != npc.mSpells.mList.end(); ++it)
        {
            stats->addSpell(*it);
        }
    }

    // update spell info
    const std::vector<std::string> &racePowers = race->mPowers.mList;
    for (unsigned int i = 0; i < racePowers.size(); ++i)
    {
        int type = -1;
        int spellIndex = mData.getSpells().searchId(racePowers[i]);
        if (spellIndex != -1)
            type = mData.getSpells().getRecord(spellIndex).get().mData.mType;
        stats->addPowers(racePowers[i], type);
    }
    // cost/chance
    int skills[ESM::Skill::Length];
    if (autoCalc)
        for (int i = 0; i< ESM::Skill::Length; ++i)
            skills[i] = stats->getBaseSkill(i);
    else
        for (int i = 0; i< ESM::Skill::Length; ++i)
            skills[i] = npc.mNpdt52.mSkills[i];

    int attributes[ESM::Attribute::Length];
    if (autoCalc)
        for (int i = 0; i< ESM::Attribute::Length; ++i)
            attributes[i] = stats->getBaseAttribute(i);
    else
    {
        attributes[ESM::Attribute::Strength]    = npc.mNpdt52.mStrength;
        attributes[ESM::Attribute::Willpower]   = npc.mNpdt52.mWillpower;
        attributes[ESM::Attribute::Agility]     = npc.mNpdt52.mAgility;
        attributes[ESM::Attribute::Speed]       = npc.mNpdt52.mSpeed;
        attributes[ESM::Attribute::Endurance]   = npc.mNpdt52.mEndurance;
        attributes[ESM::Attribute::Personality] = npc.mNpdt52.mPersonality;
        attributes[ESM::Attribute::Luck]        = npc.mNpdt52.mLuck;
    }

    const std::vector<CSMWorld::SpellInfo>& spells = stats->spells();
    for (std::vector<SpellInfo>::const_iterator it = spells.begin(); it != spells.end(); ++it)
    {
        int cost = -1;
        int spellIndex = mData.getSpells().searchId((*it).mName);
        const ESM::Spell* spell = 0;
        if (spellIndex != -1)
        {
            spell = &mData.getSpells().getRecord(spellIndex).get();
            cost = spell->mData.mCost;

            int school;
            float skillTerm;
            AutoCalc::calcWeakestSchool(spell, skills, school, skillTerm, &store);
            float chance = calcAutoCastChance(spell, skills, attributes, school, &store);

            stats->addCostAndChance((*it).mName, cost, (int)ceil(chance)); // percent
        }
    }

    if (stats.get() == 0)
        return 0;

    CSMWorld::NpcStats *result = stats.release();
    //emit cacheNpcStats (npc.mId, result);
    mNpcStatCache[npc.mId] = result;
    return result;
}

//void CSMWorld::NpcAutoCalc::cacheNpcStatsEvent (const std::string& id, CSMWorld::NpcStats *stats)
//{
    //mNpcStatCache[id] = stats;
//}

CSMWorld::NpcStats* CSMWorld::NpcAutoCalc::getCachedNpcData (const std::string& id) const
{
    std::map<const std::string, CSMWorld::NpcStats*>::const_iterator it = mNpcStatCache.find(id);
    if (it != mNpcStatCache.end())
        return it->second;
    else
        return 0;
}
