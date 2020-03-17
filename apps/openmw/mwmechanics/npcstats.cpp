#include "npcstats.hpp"

#include <iomanip>

#include <components/esm/loadclas.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/npcstats.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

MWMechanics::NpcStats::NpcStats()
    : mDisposition (0)
, mReputation(0)
, mCrimeId(-1)
, mBounty(0)
, mWerewolfKills (0)
, mLevelProgress(0)
, mTimeToStartDrowning(-1.0) // set breath to special value, it will be replaced during actor update
    , mIsWerewolf(false)
{
    mSkillIncreases.resize (ESM::Attribute::Length, 0);
    mSpecIncreases.resize(3, 0);
}

int MWMechanics::NpcStats::getBaseDisposition() const
{
    return mDisposition;
}

void MWMechanics::NpcStats::setBaseDisposition(int disposition)
{
    mDisposition = disposition;
}

const MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill (int index) const
{
    if (index<0 || index>=ESM::Skill::Length)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill (int index)
{
    if (index<0 || index>=ESM::Skill::Length)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

void MWMechanics::NpcStats::setSkill(int index, const MWMechanics::SkillValue &value)
{
    if (index<0 || index>=ESM::Skill::Length)
        throw std::runtime_error ("skill index out of range");

    mSkill[index] = value;
}

const std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks() const
{
    return mFactionRank;
}

int MWMechanics::NpcStats::getFactionRank(const std::string &faction) const
{
    const std::string lower = Misc::StringUtils::lowerCase(faction);
    std::map<std::string, int>::const_iterator it = mFactionRank.find(lower);
    if (it != mFactionRank.end())
        return it->second;

    return -1;
}

void MWMechanics::NpcStats::raiseRank(const std::string &faction)
{
    const std::string lower = Misc::StringUtils::lowerCase(faction);
    std::map<std::string, int>::iterator it = mFactionRank.find(lower);
    if (it != mFactionRank.end())
    {
        // Does the next rank exist?
        const ESM::Faction* factionPtr = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(lower);
        if (it->second+1 < 10 && !factionPtr->mRanks[it->second+1].empty())
            it->second += 1;
    }
}

void MWMechanics::NpcStats::lowerRank(const std::string &faction)
{
    const std::string lower = Misc::StringUtils::lowerCase(faction);
    std::map<std::string, int>::iterator it = mFactionRank.find(lower);
    if (it != mFactionRank.end())
    {
        it->second = it->second-1;
        if (it->second < 0)
        {
            mFactionRank.erase(it);
            mExpelled.erase(lower);
        }
    }
}

void MWMechanics::NpcStats::joinFaction(const std::string& faction)
{
    const std::string lower = Misc::StringUtils::lowerCase(faction);
    std::map<std::string, int>::iterator it = mFactionRank.find(lower);
    if (it == mFactionRank.end())
        mFactionRank[lower] = 0;
}

bool MWMechanics::NpcStats::getExpelled(const std::string& factionID) const
{
    return mExpelled.find(Misc::StringUtils::lowerCase(factionID)) != mExpelled.end();
}

void MWMechanics::NpcStats::expell(const std::string& factionID)
{
    std::string lower = Misc::StringUtils::lowerCase(factionID);
    if (mExpelled.find(lower) == mExpelled.end())
    {
        std::string message = "#{sExpelledMessage}";
        message += MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID)->mName;
        MWBase::Environment::get().getWindowManager()->messageBox(message);
        mExpelled.insert(lower);
    }
}

void MWMechanics::NpcStats::clearExpelled(const std::string& factionID)
{
    mExpelled.erase(Misc::StringUtils::lowerCase(factionID));
}

bool MWMechanics::NpcStats::isInFaction (const std::string& faction) const
{
    return (mFactionRank.find(Misc::StringUtils::lowerCase(faction)) != mFactionRank.end());
}

int MWMechanics::NpcStats::getFactionReputation (const std::string& faction) const
{
    std::map<std::string, int>::const_iterator iter = mFactionReputation.find (Misc::StringUtils::lowerCase(faction));

    if (iter==mFactionReputation.end())
        return 0;

    return iter->second;
}

void MWMechanics::NpcStats::setFactionReputation (const std::string& faction, int value)
{
    mFactionReputation[Misc::StringUtils::lowerCase(faction)] = value;
}

float MWMechanics::NpcStats::getSkillProgressRequirement (int skillIndex, const ESM::Class& class_) const
{
    float progressRequirement = static_cast<float>(1 + getSkill(skillIndex).getBase());

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    float typeFactor = gmst.find ("fMiscSkillBonus")->mValue.getFloat();

    for (int i=0; i<5; ++i)
    {
        if (class_.mData.mSkills[i][0]==skillIndex)
        {
            typeFactor = gmst.find ("fMinorSkillBonus")->mValue.getFloat();
            break;
        }
        else if (class_.mData.mSkills[i][1]==skillIndex)
        {
            typeFactor = gmst.find ("fMajorSkillBonus")->mValue.getFloat();
            break;
        }
    }

    progressRequirement *= typeFactor;

    if (typeFactor<=0)
        throw std::runtime_error ("invalid skill type factor");

    float specialisationFactor = 1;

    const ESM::Skill *skill =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find (skillIndex);
    if (skill->mData.mSpecialization==class_.mData.mSpecialization)
    {
        specialisationFactor = gmst.find ("fSpecialSkillBonus")->mValue.getFloat();

        if (specialisationFactor<=0)
            throw std::runtime_error ("invalid skill specialisation factor");
    }
    progressRequirement *= specialisationFactor;

    return progressRequirement;
}

void MWMechanics::NpcStats::useSkill (int skillIndex, const ESM::Class& class_, int usageType, float extraFactor)
{
    const ESM::Skill *skill =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find (skillIndex);
    float skillGain = 1;
    if (usageType>=4)
        throw std::runtime_error ("skill usage type out of range");
    if (usageType>=0)
    {
        skillGain = skill->mData.mUseValue[usageType];
        if (skillGain<0)
            throw std::runtime_error ("invalid skill gain factor");
    }
    skillGain *= extraFactor;

    MWMechanics::SkillValue& value = getSkill (skillIndex);

    value.setProgress(value.getProgress() + skillGain);

    if (int(value.getProgress())>=int(getSkillProgressRequirement(skillIndex, class_)))
    {
        // skill levelled up
        increaseSkill(skillIndex, class_, false);
    }
}

void MWMechanics::NpcStats::increaseSkill(int skillIndex, const ESM::Class &class_, bool preserveProgress, bool readBook)
{
    int base = getSkill (skillIndex).getBase();

    if (base >= 100)
        return;

    base += 1;

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    // is this a minor or major skill?
    int increase = gmst.find("iLevelupMiscMultAttriubte")->mValue.getInteger(); // Note: GMST has a typo
    for (int k=0; k<5; ++k)
    {
        if (class_.mData.mSkills[k][0] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMinorMult")->mValue.getInteger();
            increase = gmst.find("iLevelUpMinorMultAttribute")->mValue.getInteger();
            break;
        }
        else if (class_.mData.mSkills[k][1] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMajorMult")->mValue.getInteger();
            increase = gmst.find("iLevelUpMajorMultAttribute")->mValue.getInteger();
            break;
        }
    }

    const ESM::Skill* skill =
        MWBase::Environment::get().getWorld ()->getStore ().get<ESM::Skill>().find(skillIndex);
    mSkillIncreases[skill->mData.mAttribute] += increase;

    mSpecIncreases[skill->mData.mSpecialization] += gmst.find("iLevelupSpecialization")->mValue.getInteger();

    // Play sound & skill progress notification
    /// \todo check if character is the player, if levelling is ever implemented for NPCs
    MWBase::Environment::get().getWindowManager()->playSound("skillraise");

    std::string message = MWBase::Environment::get().getWindowManager ()->getGameSettingString ("sNotifyMessage39", "");
    message = Misc::StringUtils::format(message, ("#{" + ESM::Skill::sSkillNameIds[skillIndex] + "}"), base);

    if (readBook)
        message = "#{sBookSkillMessage}\n" + message;
    
    MWBase::Environment::get().getWindowManager ()->messageBox(message, MWGui::ShowInDialogueMode_Never);

    if (mLevelProgress >= gmst.find("iLevelUpTotal")->mValue.getInteger())
    {
        // levelup is possible now
        MWBase::Environment::get().getWindowManager ()->messageBox ("#{sLevelUpMsg}", MWGui::ShowInDialogueMode_Never);
    }

    getSkill(skillIndex).setBase (base);
    if (!preserveProgress)
        getSkill(skillIndex).setProgress(0);
}

int MWMechanics::NpcStats::getLevelProgress () const
{
    return mLevelProgress;
}

void MWMechanics::NpcStats::levelUp()
{
    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    mLevelProgress -= gmst.find("iLevelUpTotal")->mValue.getInteger();
    mLevelProgress = std::max(0, mLevelProgress); // might be necessary when levelup was invoked via console

    for (int i=0; i<ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = 0;

    const int endurance = getAttribute(ESM::Attribute::Endurance).getBase();

    // "When you gain a level, in addition to increasing three primary attributes, your Health
    // will automatically increase by 10% of your Endurance attribute. If you increased Endurance this level,
    // the Health increase is calculated from the increased Endurance"
    // Note: we should add bonus Health points to current level too.
    float healthGain = endurance * gmst.find("fLevelUpHealthEndMult")->mValue.getFloat();
    MWMechanics::DynamicStat<float> health(getHealth());
    health.setBase(getHealth().getBase() + healthGain);
    health.setCurrent(std::max(1.f, getHealth().getCurrent() + healthGain));
    setHealth(health);

    setLevel(getLevel()+1);
}

void MWMechanics::NpcStats::updateHealth()
{
    const int endurance = getAttribute(ESM::Attribute::Endurance).getBase();
    const int strength = getAttribute(ESM::Attribute::Strength).getBase();

    setHealth(floor(0.5f * (strength + endurance)));
}

int MWMechanics::NpcStats::getLevelupAttributeMultiplier(int attribute) const
{
    int num = mSkillIncreases[attribute];

    if (num == 0)
        return 1;

    num = std::min(10, num);

    // iLevelUp01Mult - iLevelUp10Mult
    std::stringstream gmst;
    gmst << "iLevelUp" << std::setfill('0') << std::setw(2) << num << "Mult";

    return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(gmst.str())->mValue.getInteger();
}

int MWMechanics::NpcStats::getSkillIncreasesForSpecialization(int spec) const
{
    return mSpecIncreases[spec];
}

void MWMechanics::NpcStats::flagAsUsed (const std::string& id)
{
    mUsedIds.insert (id);
}

bool MWMechanics::NpcStats::hasBeenUsed (const std::string& id) const
{
    return mUsedIds.find (id)!=mUsedIds.end();
}

int MWMechanics::NpcStats::getBounty() const
{
    return mBounty;
}

void MWMechanics::NpcStats::setBounty (int bounty)
{
    mBounty = bounty;
}

int MWMechanics::NpcStats::getReputation() const
{
    return mReputation;
}

void MWMechanics::NpcStats::setReputation(int reputation)
{
    // Reputation is capped in original engine
    mReputation = std::min(255, std::max(0, reputation));
}

int MWMechanics::NpcStats::getCrimeId() const
{
    return mCrimeId;
}

void MWMechanics::NpcStats::setCrimeId(int id)
{
    mCrimeId = id;
}

bool MWMechanics::NpcStats::hasSkillsForRank (const std::string& factionId, int rank) const
{
    if (rank<0 || rank>=10)
        throw std::runtime_error ("rank index out of range");

    const ESM::Faction& faction =
        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find (factionId);

    std::vector<int> skills;

    for (int i=0; i<7; ++i)
    {
        if (faction.mData.mSkills[i] != -1)
            skills.push_back (static_cast<int> (getSkill (faction.mData.mSkills[i]).getBase()));
    }

    if (skills.empty())
        return true;

    std::sort (skills.begin(), skills.end());

    std::vector<int>::const_reverse_iterator iter = skills.rbegin();

    const ESM::RankData& rankData = faction.mData.mRankData[rank];

    if (*iter<rankData.mSkill1)
        return false;

    if (skills.size() < 2)
        return true;

    return *++iter>=rankData.mSkill2;
}

bool MWMechanics::NpcStats::isWerewolf() const
{
    return mIsWerewolf;
}

void MWMechanics::NpcStats::setWerewolf (bool set)
{
    if (mIsWerewolf == set)
        return;

    if(set != false)
    {
        mWerewolfKills = 0;
    }
    mIsWerewolf = set;
}

int MWMechanics::NpcStats::getWerewolfKills() const
{
    return mWerewolfKills;
}

void MWMechanics::NpcStats::addWerewolfKill()
{
    ++mWerewolfKills;
}

float MWMechanics::NpcStats::getTimeToStartDrowning() const
{
    return mTimeToStartDrowning;
}

void MWMechanics::NpcStats::setTimeToStartDrowning(float time)
{
    mTimeToStartDrowning=time;
}

void MWMechanics::NpcStats::writeState (ESM::CreatureStats& state) const
{
    CreatureStats::writeState(state);
}

void MWMechanics::NpcStats::writeState (ESM::NpcStats& state) const
{
    for (std::map<std::string, int>::const_iterator iter (mFactionRank.begin());
        iter!=mFactionRank.end(); ++iter)
        state.mFactions[iter->first].mRank = iter->second;

    state.mDisposition = mDisposition;

    for (int i=0; i<ESM::Skill::Length; ++i)
        mSkill[i].writeState (state.mSkills[i]);

    state.mIsWerewolf = mIsWerewolf;

    state.mCrimeId = mCrimeId;

    state.mBounty = mBounty;

    for (std::set<std::string>::const_iterator iter (mExpelled.begin());
        iter!=mExpelled.end(); ++iter)
        state.mFactions[*iter].mExpelled = true;

    for (std::map<std::string, int>::const_iterator iter (mFactionReputation.begin());
        iter!=mFactionReputation.end(); ++iter)
        state.mFactions[iter->first].mReputation = iter->second;

    state.mReputation = mReputation;
    state.mWerewolfKills = mWerewolfKills;
    state.mLevelProgress = mLevelProgress;

    for (int i=0; i<ESM::Attribute::Length; ++i)
        state.mSkillIncrease[i] = mSkillIncreases[i];

    for (int i=0; i<3; ++i)
        state.mSpecIncreases[i] = mSpecIncreases[i];

    std::copy (mUsedIds.begin(), mUsedIds.end(), std::back_inserter (state.mUsedIds));

    state.mTimeToStartDrowning = mTimeToStartDrowning;
}
void MWMechanics::NpcStats::readState (const ESM::CreatureStats& state)
{
    CreatureStats::readState(state);
}

void MWMechanics::NpcStats::readState (const ESM::NpcStats& state)
{
    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

    for (std::map<std::string, ESM::NpcStats::Faction>::const_iterator iter (state.mFactions.begin());
        iter!=state.mFactions.end(); ++iter)
        if (store.get<ESM::Faction>().search (iter->first))
        {
            if (iter->second.mExpelled)
                mExpelled.insert (iter->first);

            if (iter->second.mRank >= 0)
                mFactionRank[iter->first] = iter->second.mRank;

            if (iter->second.mReputation)
                mFactionReputation[Misc::StringUtils::lowerCase(iter->first)] = iter->second.mReputation;
        }

    mDisposition = state.mDisposition;

    for (int i=0; i<ESM::Skill::Length; ++i)
        mSkill[i].readState (state.mSkills[i]);

    mIsWerewolf = state.mIsWerewolf;

    mCrimeId = state.mCrimeId;
    mBounty = state.mBounty;
    mReputation = state.mReputation;
    mWerewolfKills = state.mWerewolfKills;
    mLevelProgress = state.mLevelProgress;

    for (int i=0; i<ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = state.mSkillIncrease[i];

    for (int i=0; i<3; ++i)
        mSpecIncreases[i] = state.mSpecIncreases[i];

    for (std::vector<std::string>::const_iterator iter (state.mUsedIds.begin());
        iter!=state.mUsedIds.end(); ++iter)
        if (store.find (*iter))
            mUsedIds.insert (*iter);

    mTimeToStartDrowning = state.mTimeToStartDrowning;
}
