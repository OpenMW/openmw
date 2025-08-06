#include "npcstats.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>

#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/npcstats.hpp>

#include <components/misc/strings/format.hpp>

#include <MyGUI_TextIterator.h>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

MWMechanics::NpcStats::NpcStats()
    : mDisposition(0)
    , mCrimeDispositionModifier(0)
    , mReputation(0)
    , mCrimeId(-1)
    , mBounty(0)
    , mWerewolfKills(0)
    , mLevelProgress(0)
    , mTimeToStartDrowning(-1.0) // set breath to special value, it will be replaced during actor update
    , mIsWerewolf(false)
{
    mSpecIncreases.resize(3, 0);
    for (const ESM::Skill& skill : MWBase::Environment::get().getESMStore()->get<ESM::Skill>())
        mSkills.emplace(skill.mId, SkillValue{});
}

int MWMechanics::NpcStats::getBaseDisposition() const
{
    return mDisposition;
}

void MWMechanics::NpcStats::setBaseDisposition(int disposition)
{
    mDisposition = disposition;
}

int MWMechanics::NpcStats::getCrimeDispositionModifier() const
{
    return mCrimeDispositionModifier;
}

void MWMechanics::NpcStats::setCrimeDispositionModifier(int value)
{
    mCrimeDispositionModifier = value;
}

void MWMechanics::NpcStats::modCrimeDispositionModifier(int value)
{
    mCrimeDispositionModifier += value;
}

const MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill(ESM::RefId id) const
{
    auto it = mSkills.find(id);
    if (it == mSkills.end())
        throw std::runtime_error("skill not found");
    return it->second;
}

MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill(ESM::RefId id)
{
    auto it = mSkills.find(id);
    if (it == mSkills.end())
        throw std::runtime_error("skill not found");
    return it->second;
}

void MWMechanics::NpcStats::setSkill(ESM::RefId id, const MWMechanics::SkillValue& value)
{
    auto it = mSkills.find(id);
    if (it == mSkills.end())
        throw std::runtime_error("skill not found");
    it->second = value;
}

const std::map<ESM::RefId, int>& MWMechanics::NpcStats::getFactionRanks() const
{
    return mFactionRank;
}

int MWMechanics::NpcStats::getFactionRank(const ESM::RefId& faction) const
{
    auto it = mFactionRank.find(faction);
    if (it != mFactionRank.end())
        return it->second;

    return -1;
}

void MWMechanics::NpcStats::joinFaction(const ESM::RefId& faction)
{
    auto it = mFactionRank.find(faction);
    if (it == mFactionRank.end())
        mFactionRank[faction] = 0;
}

void MWMechanics::NpcStats::setFactionRank(const ESM::RefId& faction, int newRank)
{
    auto it = mFactionRank.find(faction);
    if (it != mFactionRank.end())
    {
        const ESM::Faction* factionPtr = MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(faction);
        if (newRank < 0)
        {
            mFactionRank.erase(it);
            mExpelled.erase(faction);
        }
        else if (newRank < static_cast<int>(factionPtr->mData.mRankData.size()))
            do
                it->second = newRank;
            // Does the new rank exist?
            while (newRank > 0 && factionPtr->mRanks[newRank--].empty());
    }
}

bool MWMechanics::NpcStats::getExpelled(const ESM::RefId& factionID) const
{
    return mExpelled.find(factionID) != mExpelled.end();
}

void MWMechanics::NpcStats::expell(const ESM::RefId& factionID, bool printMessage)
{
    if (mExpelled.find(factionID) == mExpelled.end())
    {
        mExpelled.insert(factionID);
        if (!printMessage)
            return;

        std::string message = "#{sExpelledMessage}";
        message += MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID)->mName;
        MWBase::Environment::get().getWindowManager()->messageBox(message);
    }
}

void MWMechanics::NpcStats::clearExpelled(const ESM::RefId& factionID)
{
    mExpelled.erase(factionID);
}

bool MWMechanics::NpcStats::isInFaction(const ESM::RefId& faction) const
{
    return (mFactionRank.find(faction) != mFactionRank.end());
}

int MWMechanics::NpcStats::getFactionReputation(const ESM::RefId& faction) const
{
    auto iter = mFactionReputation.find(faction);

    if (iter == mFactionReputation.end())
        return 0;

    return iter->second;
}

void MWMechanics::NpcStats::setFactionReputation(const ESM::RefId& faction, int value)
{
    mFactionReputation[faction] = value;
}

float MWMechanics::NpcStats::getSkillProgressRequirement(ESM::RefId id, const ESM::Class& npcClass) const
{
    float progressRequirement = 1.f + getSkill(id).getBase();

    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
    const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(id);

    float typeFactor = gmst.find("fMiscSkillBonus")->mValue.getFloat();
    int index = ESM::Skill::refIdToIndex(skill->mId);
    for (const auto& skills : npcClass.mData.mSkills)
    {
        if (skills[0] == index)
        {
            typeFactor = gmst.find("fMinorSkillBonus")->mValue.getFloat();
            break;
        }
        else if (skills[1] == index)
        {
            typeFactor = gmst.find("fMajorSkillBonus")->mValue.getFloat();
            break;
        }
    }

    progressRequirement *= typeFactor;

    if (typeFactor <= 0)
        throw std::runtime_error("invalid skill type factor");

    float specialisationFactor = 1;

    if (skill->mData.mSpecialization == npcClass.mData.mSpecialization)
    {
        specialisationFactor = gmst.find("fSpecialSkillBonus")->mValue.getFloat();

        if (specialisationFactor <= 0)
            throw std::runtime_error("invalid skill specialisation factor");
    }
    progressRequirement *= specialisationFactor;

    return progressRequirement;
}

int MWMechanics::NpcStats::getLevelProgress() const
{
    return mLevelProgress;
}

void MWMechanics::NpcStats::setLevelProgress(int progress)
{
    mLevelProgress = progress;
}

void MWMechanics::NpcStats::levelUp()
{
    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

    mLevelProgress -= gmst.find("iLevelUpTotal")->mValue.getInteger();
    mLevelProgress = std::max(0, mLevelProgress); // might be necessary when levelup was invoked via console

    mSkillIncreases.clear();

    const float endurance = getAttribute(ESM::Attribute::Endurance).getBase();

    // "When you gain a level, in addition to increasing three primary attributes, your Health
    // will automatically increase by 10% of your Endurance attribute. If you increased Endurance this level,
    // the Health increase is calculated from the increased Endurance"
    // Note: we should add bonus Health points to current level too.
    float healthGain = endurance * gmst.find("fLevelUpHealthEndMult")->mValue.getFloat();
    MWMechanics::DynamicStat<float> health(getHealth());
    health.setBase(getHealth().getBase() + healthGain);
    health.setCurrent(std::max(1.f, getHealth().getCurrent() + healthGain));
    setHealth(health);

    setLevel(getLevel() + 1);
}

void MWMechanics::NpcStats::updateHealth()
{
    const float endurance = getAttribute(ESM::Attribute::Endurance).getBase();
    const float strength = getAttribute(ESM::Attribute::Strength).getBase();

    setHealth(floor(0.5f * (strength + endurance)));
}

int MWMechanics::NpcStats::getLevelupAttributeMultiplier(ESM::Attribute::AttributeID attribute) const
{
    auto it = mSkillIncreases.find(attribute);
    if (it == mSkillIncreases.end() || it->second == 0)
        return 1;
    int num = std::min(10, it->second);

    // iLevelUp01Mult - iLevelUp10Mult
    std::stringstream gmst;
    gmst << "iLevelUp" << std::setfill('0') << std::setw(2) << num << "Mult";

    return MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>().find(gmst.str())->mValue.getInteger();
}

int MWMechanics::NpcStats::getSkillIncreasesForAttribute(ESM::Attribute::AttributeID attribute) const
{
    auto it = mSkillIncreases.find(attribute);
    if (it == mSkillIncreases.end())
        return 0;
    return it->second;
}

void MWMechanics::NpcStats::setSkillIncreasesForAttribute(ESM::Attribute::AttributeID attribute, int increases)
{
    if (increases == 0)
        mSkillIncreases.erase(attribute);
    else
        mSkillIncreases[attribute] = increases;
}

int MWMechanics::NpcStats::getSkillIncreasesForSpecialization(ESM::Class::Specialization spec) const
{
    return mSpecIncreases[spec];
}

void MWMechanics::NpcStats::setSkillIncreasesForSpecialization(ESM::Class::Specialization spec, int increases)
{
    assert(spec >= 0 && spec < 3);
    mSpecIncreases[spec] = increases;
}

void MWMechanics::NpcStats::flagAsUsed(const ESM::RefId& id)
{
    mUsedIds.insert(id);
}

bool MWMechanics::NpcStats::hasBeenUsed(const ESM::RefId& id) const
{
    return mUsedIds.find(id) != mUsedIds.end();
}

int MWMechanics::NpcStats::getBounty() const
{
    return mBounty;
}

void MWMechanics::NpcStats::setBounty(int bounty)
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
    mReputation = std::clamp(reputation, 0, 255);
}

int MWMechanics::NpcStats::getCrimeId() const
{
    return mCrimeId;
}

void MWMechanics::NpcStats::setCrimeId(int id)
{
    mCrimeId = id;
}

bool MWMechanics::NpcStats::hasSkillsForRank(const ESM::RefId& factionId, int rank) const
{
    const ESM::Faction& faction = *MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionId);

    const ESM::RankData& rankData = faction.mData.mRankData.at(rank);

    std::vector<int> skills;

    for (int index : faction.mData.mSkills)
    {
        ESM::RefId id = ESM::Skill::indexToRefId(index);
        if (!id.empty())
            skills.push_back(static_cast<int>(getSkill(id).getBase()));
    }

    if (skills.empty())
        return true;

    std::sort(skills.begin(), skills.end());

    std::vector<int>::const_reverse_iterator iter = skills.rbegin();

    if (*iter < rankData.mPrimarySkill)
        return false;

    if (skills.size() < 2)
        return true;

    iter++;
    if (*iter < rankData.mFavouredSkill)
        return false;

    if (skills.size() < 3)
        return true;

    iter++;
    if (*iter < rankData.mFavouredSkill)
        return false;

    return true;
}

bool MWMechanics::NpcStats::isWerewolf() const
{
    return mIsWerewolf;
}

void MWMechanics::NpcStats::setWerewolf(bool set)
{
    if (mIsWerewolf == set)
        return;

    if (set != false)
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
    mTimeToStartDrowning = time;
}

void MWMechanics::NpcStats::writeState(ESM::CreatureStats& state) const
{
    CreatureStats::writeState(state);
}

void MWMechanics::NpcStats::writeState(ESM::NpcStats& state) const
{
    for (std::map<ESM::RefId, int>::const_iterator iter(mFactionRank.begin()); iter != mFactionRank.end(); ++iter)
        state.mFactions[iter->first].mRank = iter->second;

    state.mDisposition = mDisposition;
    state.mCrimeDispositionModifier = mCrimeDispositionModifier;

    for (const auto& [id, value] : mSkills)
    {
        // TODO extend format
        auto index = ESM::Skill::refIdToIndex(id);
        assert(index >= 0);
        value.writeState(state.mSkills[static_cast<size_t>(index)]);
    }

    state.mIsWerewolf = mIsWerewolf;

    state.mCrimeId = mCrimeId;

    state.mBounty = mBounty;

    for (auto iter(mExpelled.begin()); iter != mExpelled.end(); ++iter)
        state.mFactions[*iter].mExpelled = true;

    for (auto iter(mFactionReputation.begin()); iter != mFactionReputation.end(); ++iter)
        state.mFactions[iter->first].mReputation = iter->second;

    state.mReputation = mReputation;
    state.mWerewolfKills = mWerewolfKills;
    state.mLevelProgress = mLevelProgress;

    state.mSkillIncrease.fill(0);
    for (const auto& [key, value] : mSkillIncreases)
    {
        // TODO extend format
        auto index = ESM::Attribute::refIdToIndex(key);
        assert(index >= 0);
        state.mSkillIncrease[static_cast<size_t>(index)] = value;
    }

    for (size_t i = 0; i < state.mSpecIncreases.size(); ++i)
        state.mSpecIncreases[i] = mSpecIncreases[i];

    std::copy(mUsedIds.begin(), mUsedIds.end(), std::back_inserter(state.mUsedIds));

    state.mTimeToStartDrowning = mTimeToStartDrowning;
}
void MWMechanics::NpcStats::readState(const ESM::CreatureStats& state)
{
    CreatureStats::readState(state);
}

void MWMechanics::NpcStats::readState(const ESM::NpcStats& state)
{
    const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

    for (auto iter(state.mFactions.begin()); iter != state.mFactions.end(); ++iter)
        if (store.get<ESM::Faction>().search(iter->first))
        {
            if (iter->second.mExpelled)
                mExpelled.insert(iter->first);

            if (iter->second.mRank >= 0)
                mFactionRank[iter->first] = iter->second.mRank;

            if (iter->second.mReputation)
                mFactionReputation[iter->first] = iter->second.mReputation;
        }

    mDisposition = state.mDisposition;
    mCrimeDispositionModifier = state.mCrimeDispositionModifier;

    for (size_t i = 0; i < state.mSkills.size(); ++i)
    {
        // TODO extend format
        ESM::RefId id = ESM::Skill::indexToRefId(i);
        assert(!id.empty());
        mSkills[id].readState(state.mSkills[i]);
    }

    mIsWerewolf = state.mIsWerewolf;

    mCrimeId = state.mCrimeId;
    mBounty = state.mBounty;
    mReputation = state.mReputation;
    mWerewolfKills = state.mWerewolfKills;
    mLevelProgress = state.mLevelProgress;

    for (size_t i = 0; i < state.mSkillIncrease.size(); ++i)
        mSkillIncreases[ESM::Attribute::indexToRefId(i)] = state.mSkillIncrease[i];

    for (size_t i = 0; i < state.mSpecIncreases.size(); ++i)
        mSpecIncreases[i] = state.mSpecIncreases[i];

    for (auto iter(state.mUsedIds.begin()); iter != state.mUsedIds.end(); ++iter)
        if (store.find(*iter))
            mUsedIds.insert(*iter);

    mTimeToStartDrowning = state.mTimeToStartDrowning;
}
