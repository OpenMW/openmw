#include "npcstats.hpp"

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
#include "../mwbase/world.hpp"

MWMechanics::NpcStats::NpcStats()
    : mDisposition(0)
    , mReputation(0)
    , mCrimeId(-1)
    , mBounty(0)
    , mWerewolfKills(0)
    , mLevelProgress(0)
    , mTimeToStartDrowning(-1.0) // set breath to special value, it will be replaced during actor update
    , mIsWerewolf(false)
{
    mSkillIncreases.resize(ESM::Attribute::Length, 0);
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

const MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill(int index) const
{
    if (index < 0 || index >= ESM::Skill::Length)
        throw std::runtime_error("skill index out of range");

    return mSkill[index];
}

MWMechanics::SkillValue& MWMechanics::NpcStats::getSkill(int index)
{
    if (index < 0 || index >= ESM::Skill::Length)
        throw std::runtime_error("skill index out of range");

    return mSkill[index];
}

void MWMechanics::NpcStats::setSkill(int index, const MWMechanics::SkillValue& value)
{
    if (index < 0 || index >= ESM::Skill::Length)
        throw std::runtime_error("skill index out of range");

    mSkill[index] = value;
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

void MWMechanics::NpcStats::raiseRank(const ESM::RefId& faction)
{
    auto it = mFactionRank.find(faction);
    if (it != mFactionRank.end())
    {
        // Does the next rank exist?
        const ESM::Faction* factionPtr = MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(faction);
        if (it->second + 1 < 10 && !factionPtr->mRanks[it->second + 1].empty())
            it->second += 1;
    }
}

void MWMechanics::NpcStats::lowerRank(const ESM::RefId& faction)
{
    auto it = mFactionRank.find(faction);
    if (it != mFactionRank.end())
    {
        it->second = it->second - 1;
        if (it->second < 0)
        {
            mFactionRank.erase(it);
            mExpelled.erase(faction);
        }
    }
}

void MWMechanics::NpcStats::joinFaction(const ESM::RefId& faction)
{
    auto it = mFactionRank.find(faction);
    if (it == mFactionRank.end())
        mFactionRank[faction] = 0;
}

bool MWMechanics::NpcStats::getExpelled(const ESM::RefId& factionID) const
{
    return mExpelled.find(factionID) != mExpelled.end();
}

void MWMechanics::NpcStats::expell(const ESM::RefId& factionID)
{
    if (mExpelled.find(factionID) == mExpelled.end())
    {
        std::string message = "#{sExpelledMessage}";
        message += MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID)->mName;
        MWBase::Environment::get().getWindowManager()->messageBox(message);
        mExpelled.insert(factionID);
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

float MWMechanics::NpcStats::getSkillProgressRequirement(int skillIndex, const ESM::Class& class_) const
{
    float progressRequirement = static_cast<float>(1 + getSkill(skillIndex).getBase());

    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

    float typeFactor = gmst.find("fMiscSkillBonus")->mValue.getFloat();

    for (const auto& skills : class_.mData.mSkills)
    {
        if (skills[0] == skillIndex)
        {
            typeFactor = gmst.find("fMinorSkillBonus")->mValue.getFloat();
            break;
        }
        else if (skills[1] == skillIndex)
        {
            typeFactor = gmst.find("fMajorSkillBonus")->mValue.getFloat();
            break;
        }
    }

    progressRequirement *= typeFactor;

    if (typeFactor <= 0)
        throw std::runtime_error("invalid skill type factor");

    float specialisationFactor = 1;

    const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(skillIndex);
    if (skill->mData.mSpecialization == class_.mData.mSpecialization)
    {
        specialisationFactor = gmst.find("fSpecialSkillBonus")->mValue.getFloat();

        if (specialisationFactor <= 0)
            throw std::runtime_error("invalid skill specialisation factor");
    }
    progressRequirement *= specialisationFactor;

    return progressRequirement;
}

void MWMechanics::NpcStats::useSkill(int skillIndex, const ESM::Class& class_, int usageType, float extraFactor)
{
    const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(skillIndex);
    float skillGain = 1;
    if (usageType >= 4)
        throw std::runtime_error("skill usage type out of range");
    if (usageType >= 0)
    {
        skillGain = skill->mData.mUseValue[usageType];
        if (skillGain < 0)
            throw std::runtime_error("invalid skill gain factor");
    }
    skillGain *= extraFactor;

    MWMechanics::SkillValue& value = getSkill(skillIndex);

    value.setProgress(value.getProgress() + skillGain);

    if (int(value.getProgress()) >= int(getSkillProgressRequirement(skillIndex, class_)))
    {
        // skill levelled up
        increaseSkill(skillIndex, class_, false);
    }
}

void MWMechanics::NpcStats::increaseSkill(
    int skillIndex, const ESM::Class& class_, bool preserveProgress, bool readBook)
{
    float base = getSkill(skillIndex).getBase();

    if (base >= 100.f)
        return;

    base += 1;

    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

    // is this a minor or major skill?
    int increase = gmst.find("iLevelupMiscMultAttriubte")->mValue.getInteger(); // Note: GMST has a typo
    for (const auto& skills : class_.mData.mSkills)
    {
        if (skills[0] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMinorMult")->mValue.getInteger();
            increase = gmst.find("iLevelUpMinorMultAttribute")->mValue.getInteger();
            break;
        }
        else if (skills[1] == skillIndex)
        {
            mLevelProgress += gmst.find("iLevelUpMajorMult")->mValue.getInteger();
            increase = gmst.find("iLevelUpMajorMultAttribute")->mValue.getInteger();
            break;
        }
    }

    const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(skillIndex);
    mSkillIncreases[skill->mData.mAttribute] += increase;

    mSpecIncreases[skill->mData.mSpecialization] += gmst.find("iLevelupSpecialization")->mValue.getInteger();

    // Play sound & skill progress notification
    /// \todo check if character is the player, if levelling is ever implemented for NPCs
    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("skillraise"));

    std::string message{ MWBase::Environment::get().getWindowManager()->getGameSettingString("sNotifyMessage39", {}) };
    message = Misc::StringUtils::format(
        message, MyGUI::TextIterator::toTagsString(skill->mName).asUTF8(), static_cast<int>(base));

    if (readBook)
        message = "#{sBookSkillMessage}\n" + message;

    MWBase::Environment::get().getWindowManager()->messageBox(message, MWGui::ShowInDialogueMode_Never);

    if (mLevelProgress >= gmst.find("iLevelUpTotal")->mValue.getInteger())
    {
        // levelup is possible now
        MWBase::Environment::get().getWindowManager()->messageBox("#{sLevelUpMsg}", MWGui::ShowInDialogueMode_Never);
    }

    getSkill(skillIndex).setBase(base);
    if (!preserveProgress)
        getSkill(skillIndex).setProgress(0);
}

int MWMechanics::NpcStats::getLevelProgress() const
{
    return mLevelProgress;
}

void MWMechanics::NpcStats::levelUp()
{
    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

    mLevelProgress -= gmst.find("iLevelUpTotal")->mValue.getInteger();
    mLevelProgress = std::max(0, mLevelProgress); // might be necessary when levelup was invoked via console

    for (int i = 0; i < ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = 0;

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

int MWMechanics::NpcStats::getLevelupAttributeMultiplier(int attribute) const
{
    int num = mSkillIncreases[attribute];

    if (num == 0)
        return 1;

    num = std::min(10, num);

    // iLevelUp01Mult - iLevelUp10Mult
    std::stringstream gmst;
    gmst << "iLevelUp" << std::setfill('0') << std::setw(2) << num << "Mult";

    return MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>().find(gmst.str())->mValue.getInteger();
}

int MWMechanics::NpcStats::getSkillIncreasesForSpecialization(int spec) const
{
    return mSpecIncreases[spec];
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
    if (rank < 0 || rank >= 10)
        throw std::runtime_error("rank index out of range");

    const ESM::Faction& faction = *MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionId);

    std::vector<int> skills;

    for (int i = 0; i < 7; ++i)
    {
        if (faction.mData.mSkills[i] != -1)
            skills.push_back(static_cast<int>(getSkill(faction.mData.mSkills[i]).getBase()));
    }

    if (skills.empty())
        return true;

    std::sort(skills.begin(), skills.end());

    std::vector<int>::const_reverse_iterator iter = skills.rbegin();

    const ESM::RankData& rankData = faction.mData.mRankData[rank];

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

    for (int i = 0; i < ESM::Skill::Length; ++i)
        mSkill[i].writeState(state.mSkills[i]);

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

    for (int i = 0; i < ESM::Attribute::Length; ++i)
        state.mSkillIncrease[i] = mSkillIncreases[i];

    for (int i = 0; i < 3; ++i)
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

    for (int i = 0; i < ESM::Skill::Length; ++i)
        mSkill[i].readState(state.mSkills[i]);

    mIsWerewolf = state.mIsWerewolf;

    mCrimeId = state.mCrimeId;
    mBounty = state.mBounty;
    mReputation = state.mReputation;
    mWerewolfKills = state.mWerewolfKills;
    mLevelProgress = state.mLevelProgress;

    for (int i = 0; i < ESM::Attribute::Length; ++i)
        mSkillIncreases[i] = state.mSkillIncrease[i];

    for (int i = 0; i < 3; ++i)
        mSpecIncreases[i] = state.mSpecIncreases[i];

    for (auto iter(state.mUsedIds.begin()); iter != state.mUsedIds.end(); ++iter)
        if (store.find(*iter))
            mUsedIds.insert(*iter);

    mTimeToStartDrowning = state.mTimeToStartDrowning;
}
