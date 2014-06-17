
#include "npcstats.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <iomanip>

#include <boost/format.hpp>

#include <components/esm/loadskil.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/npcstats.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

MWMechanics::NpcStats::NpcStats()
    : mBounty (0)
, mLevelProgress(0)
, mDisposition(0)
, mReputation(0)
, mCrimeId(-1)
, mWerewolfKills (0)
, mProfit(0)
, mTimeToStartDrowning(20.0)
, mLastDrowningHit(0)
, mLevelHealthBonus(0)
{
    mSkillIncreases.resize (ESM::Attribute::Length, 0);
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

    return (!mIsWerewolf ? mSkill[index] : mWerewolfSkill[index]);
}

MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill (int index)
{
    if (index<0 || index>=ESM::Skill::Length)
        throw std::runtime_error ("skill index out of range");

    return (!mIsWerewolf ? mSkill[index] : mWerewolfSkill[index]);
}

const std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks() const
{
    return mFactionRank;
}

std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks()
{
    return mFactionRank;
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

bool MWMechanics::NpcStats::isSameFaction (const NpcStats& npcStats) const
{
    for (std::map<std::string, int>::const_iterator iter (mFactionRank.begin()); iter!=mFactionRank.end();
        ++iter)
        if (npcStats.mFactionRank.find (iter->first)!=npcStats.mFactionRank.end())
            return true;

    return false;
}

float MWMechanics::NpcStats::getSkillGain (int skillIndex, const ESM::Class& class_, int usageType,
    int level) const
{
    if (level<0)
        level = static_cast<int> (getSkill (skillIndex).getBase());

    const ESM::Skill *skill =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find (skillIndex);

    float skillFactor = 1;

    if (usageType>=4)
        throw std::runtime_error ("skill usage type out of range");

    if (usageType>=0)
    {
        skillFactor = skill->mData.mUseValue[usageType];

        if (skillFactor<0)
            throw std::runtime_error ("invalid skill gain factor");

        if (skillFactor==0)
            return 0;
    }

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    float typeFactor = gmst.find ("fMiscSkillBonus")->getFloat();

    for (int i=0; i<5; ++i)
        if (class_.mData.mSkills[i][0]==skillIndex)
        {
            typeFactor = gmst.find ("fMinorSkillBonus")->getFloat();

            break;
        }

    for (int i=0; i<5; ++i)
        if (class_.mData.mSkills[i][1]==skillIndex)
        {
            typeFactor = gmst.find ("fMajorSkillBonus")->getFloat();

            break;
        }

    if (typeFactor<=0)
        throw std::runtime_error ("invalid skill type factor");

    float specialisationFactor = 1;

    if (skill->mData.mSpecialization==class_.mData.mSpecialization)
    {
        specialisationFactor = gmst.find ("fSpecialSkillBonus")->getFloat();

        if (specialisationFactor<=0)
            throw std::runtime_error ("invalid skill specialisation factor");
    }
    return 1.0 / ((level+1) * (1.0/skillFactor) * typeFactor * specialisationFactor);
}

void MWMechanics::NpcStats::useSkill (int skillIndex, const ESM::Class& class_, int usageType)
{
    // Don't increase skills as a werewolf
    if(mIsWerewolf)
        return;

    MWMechanics::SkillValue& value = getSkill (skillIndex);

    value.setProgress(value.getProgress() + getSkillGain (skillIndex, class_, usageType));

    if (value.getProgress()>=1)
    {
        // skill leveled up
        increaseSkill(skillIndex, class_, false);
    }
}

void MWMechanics::NpcStats::increaseSkill(int skillIndex, const ESM::Class &class_, bool preserveProgress)
{
    int base = getSkill (skillIndex).getBase();

    if (base >= 100)
        return;

    base += 1;

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    // is this a minor or major skill?
    int increase = gmst.find("iLevelupMiscMultAttriubte")->getInt(); // Note: GMST has a typo
    for (int k=0; k<5; ++k)
    {
        if (class_.mData.mSkills[k][0] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMinorMult")->getInt();
            increase = gmst.find("iLevelUpMajorMultAttribute")->getInt();
        }
    }
    for (int k=0; k<5; ++k)
    {
        if (class_.mData.mSkills[k][1] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMajorMult")->getInt();
            increase = gmst.find("iLevelUpMinorMultAttribute")->getInt();
        }
    }

    const ESM::Skill* skill =
        MWBase::Environment::get().getWorld ()->getStore ().get<ESM::Skill>().find(skillIndex);
    mSkillIncreases[skill->mData.mAttribute] += increase;

    // Play sound & skill progress notification
    /// \todo check if character is the player, if levelling is ever implemented for NPCs
    MWBase::Environment::get().getSoundManager ()->playSound ("skillraise", 1, 1);

    std::vector <std::string> noButtons;

    std::stringstream message;
    message << boost::format(MWBase::Environment::get().getWindowManager ()->getGameSettingString ("sNotifyMessage39", ""))
               % std::string("#{" + ESM::Skill::sSkillNameIds[skillIndex] + "}")
               % static_cast<int> (base);
    MWBase::Environment::get().getWindowManager ()->messageBox(message.str(), noButtons, MWGui::ShowInDialogueMode_Never);

    if (mLevelProgress >= gmst.find("iLevelUpTotal")->getInt())
    {
        // levelup is possible now
        MWBase::Environment::get().getWindowManager ()->messageBox ("#{sLevelUpMsg}", noButtons, MWGui::ShowInDialogueMode_Never);
    }

    getSkill (skillIndex).setBase (base);
    if (!preserveProgress)
        getSkill(skillIndex).setProgress(0);
}

int MWMechanics::NpcStats::getLevelProgress () const
{
    return mLevelProgress;
}

void MWMechanics::NpcStats::levelUp()
{
    mLevelProgress -= 10;
    for (int i=0; i<ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = 0;

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    const int endurance = getAttribute(ESM::Attribute::Endurance).getBase();

    // "When you gain a level, in addition to increasing three primary attributes, your Health
    // will automatically increase by 10% of your Endurance attribute. If you increased Endurance this level,
    // the Health increase is calculated from the increased Endurance"
    mLevelHealthBonus += endurance * gmst.find("fLevelUpHealthEndMult")->getFloat();
    updateHealth();

    setLevel(getLevel()+1);
}

void MWMechanics::NpcStats::updateHealth()
{
    const int endurance = getAttribute(ESM::Attribute::Endurance).getBase();
    const int strength = getAttribute(ESM::Attribute::Strength).getBase();

    setHealth(static_cast<int> (0.5 * (strength + endurance)) + mLevelHealthBonus);
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

    return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(gmst.str())->getInt();
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
    if (mIsWerewolf)
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iWereWolfBounty")->getInt();
    else
        return mBounty;
}

void MWMechanics::NpcStats::setBounty (int bounty)
{
    if (!mIsWerewolf)
        mBounty = bounty;
}

int MWMechanics::NpcStats::getFactionReputation (const std::string& faction) const
{
    std::map<std::string, int>::const_iterator iter = mFactionReputation.find (faction);

    if (iter==mFactionReputation.end())
        return 0;

    return iter->second;
}

void MWMechanics::NpcStats::setFactionReputation (const std::string& faction, int value)
{
    mFactionReputation[faction] = value;
}

int MWMechanics::NpcStats::getReputation() const
{
    return mReputation;
}

void MWMechanics::NpcStats::setReputation(int reputation)
{
    mReputation = reputation;
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
            skills.push_back (static_cast<int> (getSkill (faction.mData.mSkills[i]).getModified()));
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
    if(set != false)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        mWerewolfKills = 0;

        for(size_t i = 0;i < ESM::Attribute::Length;i++)
        {
            mWerewolfAttributes[i] = getAttribute(i);
            // Oh, Bethesda. It's "Intelligence".
            std::string name = "fWerewolf"+((i==ESM::Attribute::Intelligence) ? std::string("Intellegence") :
                                            ESM::Attribute::sAttributeNames[i]);
            mWerewolfAttributes[i].setBase(int(gmst.find(name)->getFloat()));
        }

        for(size_t i = 0;i < ESM::Skill::Length;i++)
        {
            mWerewolfSkill[i] = getSkill(i);

            // Acrobatics is set separately for some reason.
            if(i == ESM::Skill::Acrobatics)
                continue;

            // "Mercantile"! >_<
            std::string name = "fWerewolf"+((i==ESM::Skill::Mercantile) ? std::string("Merchantile") :
                                            ESM::Skill::sSkillNames[i]);
            mWerewolfSkill[i].setBase(int(gmst.find(name)->getFloat()));
        }
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

int MWMechanics::NpcStats::getProfit() const
{
    return mProfit;
}

void MWMechanics::NpcStats::modifyProfit(int diff)
{
    mProfit += diff;
}

float MWMechanics::NpcStats::getTimeToStartDrowning() const
{
    return mTimeToStartDrowning;
}

void MWMechanics::NpcStats::setTimeToStartDrowning(float time)
{
    mTimeToStartDrowning=time;
}

void MWMechanics::NpcStats::writeState (ESM::NpcStats& state) const
{
    for (std::map<std::string, int>::const_iterator iter (mFactionRank.begin());
        iter!=mFactionRank.end(); ++iter)
        state.mFactions[iter->first].mRank = iter->second;

    state.mDisposition = mDisposition;

    for (int i=0; i<ESM::Skill::Length; ++i)
    {
        mSkill[i].writeState (state.mSkills[i].mRegular);
        mWerewolfSkill[i].writeState (state.mSkills[i].mWerewolf);
    }
    for (int i=0; i<ESM::Attribute::Length; ++i)
    {
        mWerewolfAttributes[i].writeState (state.mWerewolfAttributes[i]);
    }
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
    state.mProfit = mProfit;
    state.mLevelProgress = mLevelProgress;

    for (int i=0; i<ESM::Attribute::Length; ++i)
        state.mSkillIncrease[i] = mSkillIncreases[i];

    std::copy (mUsedIds.begin(), mUsedIds.end(), std::back_inserter (state.mUsedIds));

    state.mTimeToStartDrowning = mTimeToStartDrowning;
    state.mLastDrowningHit = mLastDrowningHit;
    state.mLevelHealthBonus = mLevelHealthBonus;
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
                mFactionReputation[iter->first] = iter->second.mReputation;
        }

    mDisposition = state.mDisposition;

    for (int i=0; i<ESM::Skill::Length; ++i)
    {
        mSkill[i].readState (state.mSkills[i].mRegular);
        mWerewolfSkill[i].readState (state.mSkills[i].mWerewolf);
    }
    for (int i=0; i<ESM::Attribute::Length; ++i)
    {
        mWerewolfAttributes[i].readState (state.mWerewolfAttributes[i]);
    }

    mIsWerewolf = state.mIsWerewolf;

    mCrimeId = state.mCrimeId;
    mBounty = state.mBounty;
    mReputation = state.mReputation;
    mWerewolfKills = state.mWerewolfKills;
    mProfit = state.mProfit;
    mLevelProgress = state.mLevelProgress;

    for (int i=0; i<ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = state.mSkillIncrease[i];

    for (std::vector<std::string>::const_iterator iter (state.mUsedIds.begin());
        iter!=state.mUsedIds.end(); ++iter)
        if (store.find (*iter))
            mUsedIds.insert (*iter);

    mTimeToStartDrowning = state.mTimeToStartDrowning;
    mLastDrowningHit = state.mLastDrowningHit;
    mLevelHealthBonus = state.mLevelHealthBonus;
}
