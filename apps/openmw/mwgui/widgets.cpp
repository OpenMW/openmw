#include "widgets.hpp"
#include "window_manager.hpp"
#include "components/esm_store/store.hpp"

#include <boost/lexical_cast.hpp>

#undef min
#undef max

using namespace MWGui;
using namespace MWGui::Widgets;

/* Helper functions */

/*
 * Fixes the filename of a texture path to use the correct .dds extension.
 * This is needed on some ESM entries which point to a .tga file instead.
 */
void MWGui::Widgets::fixTexturePath(std::string &path)
{
    int offset = path.rfind(".");
    if (offset < 0)
        return;
    path.replace(offset, path.length() - offset, ".dds");
}

/* MWSkill */

MWSkill::MWSkill()
    : mManager(nullptr)
    , mSkillId(ESM::Skill::Length)
    , mSkillNameWidget(nullptr)
    , mSkillValueWidget(nullptr)
{
}

void MWSkill::setSkillId(ESM::Skill::SkillEnum skill)
{
    mSkillId = skill;
    updateWidgets();
}

void MWSkill::setSkillNumber(int skill)
{
    if (skill < 0)
        setSkillId(ESM::Skill::Length);
    else if (skill < ESM::Skill::Length)
        setSkillId(static_cast<ESM::Skill::SkillEnum>(skill));
    else
        throw new std::runtime_error("Skill number out of range");
}

void MWSkill::setSkillValue(const SkillValue& value)
{
    mValue = value;
    updateWidgets();
}

void MWSkill::updateWidgets()
{
    if (mSkillNameWidget && mManager)
    {
        if (mSkillId == ESM::Skill::Length)
        {
            static_cast<MyGUI::TextBox*>(mSkillNameWidget)->setCaption("");
        }
        else
        {
            const std::string &name = mManager->getGameSettingString(ESM::Skill::sSkillNameIds[mSkillId], "");
            static_cast<MyGUI::TextBox*>(mSkillNameWidget)->setCaption(name);
        }
    }
    if (mSkillValueWidget)
    {
        SkillValue::Type modified = mValue.getModified(), base = mValue.getBase();
        static_cast<MyGUI::TextBox*>(mSkillValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            mSkillValueWidget->_setWidgetState("increased");
        else if (modified < base)
            mSkillValueWidget->_setWidgetState("decreased");
        else
            mSkillValueWidget->_setWidgetState("normal");
    }
}

void MWSkill::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

MWSkill::~MWSkill()
{
}

void MWSkill::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mSkillNameWidget, "StatName");
    assignWidget(mSkillValueWidget, "StatValue");

    MyGUI::ButtonPtr button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        mSkillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        mSkillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }
}

/* MWAttribute */

MWAttribute::MWAttribute()
    : mManager(nullptr)
    , mId(-1)
    , mAttributeNameWidget(nullptr)
    , mAttributeValueWidget(nullptr)
{
}

void MWAttribute::setAttributeId(int attributeId)
{
    mId = attributeId;
    updateWidgets();
}

void MWAttribute::setAttributeValue(const AttributeValue& value)
{
    mValue = value;
    updateWidgets();
}

void MWAttribute::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

void MWAttribute::updateWidgets()
{
    if (mAttributeNameWidget && mManager)
    {
        if (mId < 0 || mId >= 8)
        {
            static_cast<MyGUI::TextBox*>(mAttributeNameWidget)->setCaption("");
        }
        else
        {
            static const char *attributes[8] = {
                "sAttributeStrength",
                "sAttributeIntelligence",
                "sAttributeWillpower",
                "sAttributeAgility",
                "sAttributeSpeed",
                "sAttributeEndurance",
                "sAttributePersonality",
                "sAttributeLuck"
            };
            const std::string &name = mManager->getGameSettingString(attributes[mId], "");
            static_cast<MyGUI::TextBox*>(mAttributeNameWidget)->setCaption(name);
        }
    }
    if (mAttributeValueWidget)
    {
        AttributeValue::Type modified = mValue.getModified(), base = mValue.getBase();
        static_cast<MyGUI::TextBox*>(mAttributeValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            mAttributeValueWidget->_setWidgetState("increased");
        else if (modified < base)
            mAttributeValueWidget->_setWidgetState("decreased");
        else
            mAttributeValueWidget->_setWidgetState("normal");
    }
}

MWAttribute::~MWAttribute()
{
}

void MWAttribute::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mAttributeNameWidget, "StatName");
    assignWidget(mAttributeValueWidget, "StatValue");
    
    MyGUI::ButtonPtr button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        mAttributeNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        mAttributeValueWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }
}

/* MWSpell */

MWSpell::MWSpell()
    : mWindowManager(nullptr)
    , mSpellNameWidget(nullptr)
{
}

void MWSpell::setSpellId(const std::string &spellId)
{
    mId = spellId;
    updateWidgets();
}

void MWSpell::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, int flags)
{
    const ESMS::ESMStore &store = mWindowManager->getStore();
    const ESM::Spell *spell = store.spells.search(mId);
    MYGUI_ASSERT(spell, "spell with id '" << mId << "' not found");

    MWSpellEffectPtr effect = nullptr;
    std::vector<ESM::ENAMstruct>::const_iterator end = spell->effects.list.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->effects.list.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        SpellEffectParams params;
        params.mEffectID = it->effectID;
        params.mSkill = it->skill;
        params.mAttribute = it->attribute;
        params.mDuration = it->duration;
        params.mMagnMin = it->magnMin;
        params.mMagnMax = it->magnMax;
        params.mRange = it->range;
        params.mIsConstant = (flags & MWEffectList::EF_Constant);
        params.mNoTarget = (flags & MWEffectList::EF_NoTarget);
        effect->setSpellEffect(params);
        effects.push_back(effect);
        coord.top += effect->getHeight();
        coord.width = std::max(coord.width, effect->getRequestedWidth());
    }
}

void MWSpell::updateWidgets()
{
    if (mSpellNameWidget && mWindowManager)
    {
        const ESMS::ESMStore &store = mWindowManager->getStore();
        const ESM::Spell *spell = store.spells.search(mId);
        if (spell)
            static_cast<MyGUI::TextBox*>(mSpellNameWidget)->setCaption(spell->name);
        else
            static_cast<MyGUI::TextBox*>(mSpellNameWidget)->setCaption("");
    }
}

void MWSpell::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mSpellNameWidget, "StatName");
}

MWSpell::~MWSpell()
{
}

/* MWEffectList */

MWEffectList::MWEffectList()
    : mWindowManager(nullptr)
    , mEffectList(0)
{
}

void MWEffectList::setEffectList(const SpellEffectList& list)
{
    mEffectList = list;
    updateWidgets();
}

void MWEffectList::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, bool center, int flags)
{
    // We don't know the width of all the elements beforehand, so we do it in
    // 2 steps: first, create all widgets and check their width....
    MWSpellEffectPtr effect = nullptr;
    int maxwidth = coord.width;

    for (SpellEffectList::iterator it=mEffectList.begin();
        it != mEffectList.end(); ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        it->mIsConstant = (flags & EF_Constant) || it->mIsConstant;
        it->mNoTarget = (flags & EF_NoTarget) || it->mNoTarget;
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        if (effect->getRequestedWidth() > maxwidth)
            maxwidth = effect->getRequestedWidth();

        coord.top += effect->getHeight();
    }

    // ... then adjust the size for all widgets
    for (std::vector<MyGUI::WidgetPtr>::iterator it = effects.begin(); it != effects.end(); ++it)
    {
        effect = static_cast<MWSpellEffectPtr>(*it);
        bool needcenter = center && (maxwidth > effect->getRequestedWidth());
        int diff = maxwidth - effect->getRequestedWidth();
        if (needcenter)
        {
            effect->setCoord(diff/2, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
        }
        else
        {
            effect->setCoord(0, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
        }
    }

    // inform the parent about width
    coord.width = maxwidth;
}

void MWEffectList::updateWidgets()
{
}

void MWEffectList::initialiseOverride()
{
    Base::initialiseOverride();
}

MWEffectList::~MWEffectList()
{
}

SpellEffectList MWEffectList::effectListFromESM(const ESM::EffectList* effects)
{
    SpellEffectList result;
    std::vector<ESM::ENAMstruct>::const_iterator end = effects->list.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = effects->list.begin(); it != end; ++it)
    {
        SpellEffectParams params;
        params.mEffectID = it->effectID;
        params.mSkill = it->skill;
        params.mAttribute = it->attribute;
        params.mDuration = it->duration;
        params.mMagnMin = it->magnMin;
        params.mMagnMax = it->magnMax;
        params.mRange = it->range;
        result.push_back(params);
    }
    return result;
}

/* MWSpellEffect */

MWSpellEffect::MWSpellEffect()
    : mWindowManager(nullptr)
    , mImageWidget(nullptr)
    , mTextWidget(nullptr)
    , mRequestedWidth(0)
{
}

void MWSpellEffect::setSpellEffect(const SpellEffectParams& params)
{
    mEffectParams = params;
    updateWidgets();
}

void MWSpellEffect::updateWidgets()
{
    if (!mWindowManager)
        return;

    const ESMS::ESMStore &store = mWindowManager->getStore();
    const ESM::MagicEffect *magicEffect = store.magicEffects.search(mEffectParams.mEffectID);
    if (!magicEffect)
        return;
    if (mTextWidget)
    {
        std::string pt =  mWindowManager->getGameSettingString("spoint", "");
        std::string pts =  mWindowManager->getGameSettingString("spoints", "");
        std::string to =  " " + mWindowManager->getGameSettingString("sTo", "") + " ";
        std::string sec =  " " + mWindowManager->getGameSettingString("ssecond", "");
        std::string secs =  " " + mWindowManager->getGameSettingString("sseconds", "");

        std::string effectIDStr = effectIDToString(mEffectParams.mEffectID);
        std::string spellLine = mWindowManager->getGameSettingString(effectIDStr, "");
        if (effectInvolvesSkill(effectIDStr) && mEffectParams.mSkill >= 0 && mEffectParams.mSkill < ESM::Skill::Length)
        {
            spellLine += " " + mWindowManager->getGameSettingString(ESM::Skill::sSkillNameIds[mEffectParams.mSkill], "");
        }
        if (effectInvolvesAttribute(effectIDStr) && mEffectParams.mAttribute >= 0 && mEffectParams.mAttribute < 8)
        {
            static const char *attributes[8] = {
                "sAttributeStrength",
                "sAttributeIntelligence",
                "sAttributeWillpower",
                "sAttributeAgility",
                "sAttributeSpeed",
                "sAttributeEndurance",
                "sAttributePersonality",
                "sAttributeLuck"
            };
            spellLine += " " + mWindowManager->getGameSettingString(attributes[mEffectParams.mAttribute], "");
        }

        if ((mEffectParams.mMagnMin >= 0 || mEffectParams.mMagnMax >= 0) && effectHasMagnitude(effectIDStr))
        {
            if (mEffectParams.mMagnMin == mEffectParams.mMagnMax)
                spellLine += " " + boost::lexical_cast<std::string>(mEffectParams.mMagnMin) + " " + ((mEffectParams.mMagnMin == 1) ? pt : pts);
            else
            {
                spellLine += " " + boost::lexical_cast<std::string>(mEffectParams.mMagnMin) + to + boost::lexical_cast<std::string>(mEffectParams.mMagnMax) + " " + pts;
            }
        }

        // constant effects have no duration and no target
        if (!mEffectParams.mIsConstant)
        {
            if (mEffectParams.mDuration >= 0 && effectHasDuration(effectIDStr))
            {
                spellLine += " " + mWindowManager->getGameSettingString("sfor", "") + " " + boost::lexical_cast<std::string>(mEffectParams.mDuration) + ((mEffectParams.mDuration == 1) ? sec : secs);
            }

            // potions have no target
            if (!mEffectParams.mNoTarget)
            {
                std::string on = mWindowManager->getGameSettingString("sonword", "");
                if (mEffectParams.mRange == ESM::RT_Self)
                    spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeSelf", "");
                else if (mEffectParams.mRange == ESM::RT_Touch)
                    spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTouch", "");
                else if (mEffectParams.mRange == ESM::RT_Target)
                    spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTarget", "");
            }
        }

        static_cast<MyGUI::TextBox*>(mTextWidget)->setCaption(spellLine);
        mRequestedWidth = mTextWidget->getTextSize().width + 24;
    }
    if (mImageWidget)
    {
        std::string path = std::string("icons\\") + magicEffect->icon;
        fixTexturePath(path);
        mImageWidget->setImageTexture(path);
    }
}

std::string MWSpellEffect::effectIDToString(const short effectID)
{
    // Map effect ID to GMST name
    // http://www.uesp.net/morrow/hints/mweffects.shtml
    std::map<short, std::string> names;
    names[85] ="sEffectAbsorbAttribute";
    names[88] ="sEffectAbsorbFatigue";
    names[86] ="sEffectAbsorbHealth";
    names[87] ="sEffectAbsorbSpellPoints";
    names[89] ="sEffectAbsorbSkill";
    names[63] ="sEffectAlmsiviIntervention";
    names[47] ="sEffectBlind";
    names[123] ="sEffectBoundBattleAxe";
    names[129] ="sEffectBoundBoots";
    names[127] ="sEffectBoundCuirass";
    names[120] ="sEffectBoundDagger";
    names[131] ="sEffectBoundGloves";
    names[128] ="sEffectBoundHelm";
    names[125] ="sEffectBoundLongbow";
    names[121] ="sEffectBoundLongsword";
    names[122] ="sEffectBoundMace";
    names[130] ="sEffectBoundShield";
    names[124] ="sEffectBoundSpear";
    names[7] ="sEffectBurden";
    names[50] ="sEffectCalmCreature";
    names[49] ="sEffectCalmHumanoid";
    names[40] ="sEffectChameleon";
    names[44] ="sEffectCharm";
    names[118] ="sEffectCommandCreatures";
    names[119] ="sEffectCommandHumanoids";
    names[132] ="sEffectCorpus"; // NB this typo. (bethesda made it)
    names[70] ="sEffectCureBlightDisease";
    names[69] ="sEffectCureCommonDisease";
    names[71] ="sEffectCureCorprusDisease";
    names[73] ="sEffectCureParalyzation";
    names[72] ="sEffectCurePoison";
    names[22] ="sEffectDamageAttribute";
    names[25] ="sEffectDamageFatigue";
    names[23] ="sEffectDamageHealth";
    names[24] ="sEffectDamageMagicka";
    names[26] ="sEffectDamageSkill";
    names[54] ="sEffectDemoralizeCreature";
    names[53] ="sEffectDemoralizeHumanoid";
    names[64] ="sEffectDetectAnimal";
    names[65] ="sEffectDetectEnchantment";
    names[66] ="sEffectDetectKey";
    names[38] ="sEffectDisintegrateArmor";
    names[37] ="sEffectDisintegrateWeapon";
    names[57] ="sEffectDispel";
    names[62] ="sEffectDivineIntervention";
    names[17] ="sEffectDrainAttribute";
    names[20] ="sEffectDrainFatigue";
    names[18] ="sEffectDrainHealth";
    names[19] ="sEffectDrainSpellpoints";
    names[21] ="sEffectDrainSkill";
    names[8] ="sEffectFeather";
    names[14] ="sEffectFireDamage";
    names[4] ="sEffectFireShield";
    names[117] ="sEffectFortifyAttackBonus";
    names[79] ="sEffectFortifyAttribute";
    names[82] ="sEffectFortifyFatigue";
    names[80] ="sEffectFortifyHealth";
    names[81] ="sEffectFortifySpellpoints";
    names[84] ="sEffectFortifyMagickaMultiplier";
    names[83] ="sEffectFortifySkill";
    names[52] ="sEffectFrenzyCreature";
    names[51] ="sEffectFrenzyHumanoid";
    names[16] ="sEffectFrostDamage";
    names[6] ="sEffectFrostShield";
    names[39] ="sEffectInvisibility";
    names[9] ="sEffectJump";
    names[10] ="sEffectLevitate";
    names[41] ="sEffectLight";
    names[5] ="sEffectLightningShield";
    names[12] ="sEffectLock";
    names[60] ="sEffectMark";
    names[43] ="sEffectNightEye";
    names[13] ="sEffectOpen";
    names[45] ="sEffectParalyze";
    names[27] ="sEffectPoison";
    names[56] ="sEffectRallyCreature";
    names[55] ="sEffectRallyHumanoid";
    names[61] ="sEffectRecall";
    names[68] ="sEffectReflect";
    names[100] ="sEffectRemoveCurse";
    names[95] ="sEffectResistBlightDisease";
    names[94] ="sEffectResistCommonDisease";
    names[96] ="sEffectResistCorprusDisease";
    names[90] ="sEffectResistFire";
    names[91] ="sEffectResistFrost";
    names[93] ="sEffectResistMagicka";
    names[98] ="sEffectResistNormalWeapons";
    names[99] ="sEffectResistParalysis";
    names[97] ="sEffectResistPoison";
    names[92] ="sEffectResistShock";
    names[74] ="sEffectRestoreAttribute";
    names[77] ="sEffectRestoreFatigue";
    names[75] ="sEffectRestoreHealth";
    names[76] ="sEffectRestoreSpellPoints";
    names[78] ="sEffectRestoreSkill";
    names[42] ="sEffectSanctuary";
    names[3] ="sEffectShield";
    names[15] ="sEffectShockDamage";
    names[46] ="sEffectSilence";
    names[11] ="sEffectSlowFall";
    names[58] ="sEffectSoultrap";
    names[48] ="sEffectSound";
    names[67] ="sEffectSpellAbsorption";
    names[136] ="sEffectStuntedMagicka";
    names[106] ="sEffectSummonAncestralGhost";
    names[110] ="sEffectSummonBonelord";
    names[108] ="sEffectSummonLeastBonewalker";
    names[134] ="sEffectSummonCenturionSphere";
    names[103] ="sEffectSummonClannfear";
    names[104] ="sEffectSummonDaedroth";
    names[105] ="sEffectSummonDremora";
    names[114] ="sEffectSummonFlameAtronach";
    names[115] ="sEffectSummonFrostAtronach";
    names[113] ="sEffectSummonGoldenSaint";
    names[109] ="sEffectSummonGreaterBonewalker";
    names[112] ="sEffectSummonHunger";
    names[102] ="sEffectSummonScamp";
    names[107] ="sEffectSummonSkeletalMinion";
    names[116] ="sEffectSummonStormAtronach";
    names[111] ="sEffectSummonWingedTwilight";
    names[135] ="sEffectSunDamage";
    names[1] ="sEffectSwiftSwim";
    names[59] ="sEffectTelekinesis";
    names[101] ="sEffectTurnUndead";
    names[133] ="sEffectVampirism";
    names[0] ="sEffectWaterBreathing";
    names[2] ="sEffectWaterWalking";
    names[33] ="sEffectWeaknesstoBlightDisease";
    names[32] ="sEffectWeaknesstoCommonDisease";
    names[34] ="sEffectWeaknesstoCorprusDisease";
    names[28] ="sEffectWeaknesstoFire";
    names[29] ="sEffectWeaknesstoFrost";
    names[31] ="sEffectWeaknesstoMagicka";
    names[36] ="sEffectWeaknesstoNormalWeapons";
    names[35] ="sEffectWeaknesstoPoison";
    names[30] ="sEffectWeaknesstoShock";

    // bloodmoon
    names[138] ="sEffectSummonCreature01";
    names[139] ="sEffectSummonCreature02";
    names[140] ="sEffectSummonCreature03";
    names[141] ="sEffectSummonCreature04";
    names[142] ="sEffectSummonCreature05";

    // tribunal
    names[137] ="sEffectSummonFabricant";

    assert(names.find(effectID) != names.end() && "Unimplemented effect type");

    return names[effectID];
}

bool MWSpellEffect::effectHasDuration(const std::string& effect)
{
    // lists effects that have no duration (e.g. open lock)
    std::vector<std::string> effectsWithoutDuration;
    effectsWithoutDuration.push_back("sEffectOpen");
    effectsWithoutDuration.push_back("sEffectLock");
    effectsWithoutDuration.push_back("sEffectDispel");
    effectsWithoutDuration.push_back("sEffectSunDamage");
    effectsWithoutDuration.push_back("sEffectCorpus");
    effectsWithoutDuration.push_back("sEffectVampirism");
    effectsWithoutDuration.push_back("sEffectMark");
    effectsWithoutDuration.push_back("sEffectRecall");
    effectsWithoutDuration.push_back("sEffectDivineIntervention");
    effectsWithoutDuration.push_back("sEffectAlmsiviIntervention");
    effectsWithoutDuration.push_back("sEffectCureCommonDisease");
    effectsWithoutDuration.push_back("sEffectCureBlightDisease");
    effectsWithoutDuration.push_back("sEffectCureCorprusDisease");
    effectsWithoutDuration.push_back("sEffectCurePoison");
    effectsWithoutDuration.push_back("sEffectCureParalyzation");
    effectsWithoutDuration.push_back("sEffectRemoveCurse");
    effectsWithoutDuration.push_back("sEffectRestoreAttribute");

    return (std::find(effectsWithoutDuration.begin(), effectsWithoutDuration.end(), effect) == effectsWithoutDuration.end());
}

bool MWSpellEffect::effectHasMagnitude(const std::string& effect)
{
    // lists effects that have no magnitude (e.g. invisiblity)
    std::vector<std::string> effectsWithoutMagnitude;
    effectsWithoutMagnitude.push_back("sEffectInvisibility");
    effectsWithoutMagnitude.push_back("sEffectStuntedMagicka");
    effectsWithoutMagnitude.push_back("sEffectParalyze");
    effectsWithoutMagnitude.push_back("sEffectSoultrap");
    effectsWithoutMagnitude.push_back("sEffectSilence");
    effectsWithoutMagnitude.push_back("sEffectParalyze");
    effectsWithoutMagnitude.push_back("sEffectInvisibility");
    effectsWithoutMagnitude.push_back("sEffectWaterWalking");
    effectsWithoutMagnitude.push_back("sEffectWaterBreathing");
    effectsWithoutMagnitude.push_back("sEffectSummonScamp");
    effectsWithoutMagnitude.push_back("sEffectSummonClannfear");
    effectsWithoutMagnitude.push_back("sEffectSummonDaedroth");
    effectsWithoutMagnitude.push_back("sEffectSummonDremora");
    effectsWithoutMagnitude.push_back("sEffectSummonAncestralGhost");
    effectsWithoutMagnitude.push_back("sEffectSummonSkeletalMinion");
    effectsWithoutMagnitude.push_back("sEffectSummonBonewalker");
    effectsWithoutMagnitude.push_back("sEffectSummonGreaterBonewalker");
    effectsWithoutMagnitude.push_back("sEffectSummonBonelord");
    effectsWithoutMagnitude.push_back("sEffectSummonWingedTwilight");
    effectsWithoutMagnitude.push_back("sEffectSummonHunger");
    effectsWithoutMagnitude.push_back("sEffectSummonGoldenSaint");
    effectsWithoutMagnitude.push_back("sEffectSummonFlameAtronach");
    effectsWithoutMagnitude.push_back("sEffectSummonFrostAtronach");
    effectsWithoutMagnitude.push_back("sEffectSummonStormAtronach");
    effectsWithoutMagnitude.push_back("sEffectSummonCenturionSphere");
    effectsWithoutMagnitude.push_back("sEffectBoundDagger");
    effectsWithoutMagnitude.push_back("sEffectBoundLongsword");
    effectsWithoutMagnitude.push_back("sEffectBoundMace");
    effectsWithoutMagnitude.push_back("sEffectBoundBattleAxe");
    effectsWithoutMagnitude.push_back("sEffectBoundSpear");
    effectsWithoutMagnitude.push_back("sEffectBoundLongbow");
    effectsWithoutMagnitude.push_back("sEffectBoundCuirass");
    effectsWithoutMagnitude.push_back("sEffectBoundHelm");
    effectsWithoutMagnitude.push_back("sEffectBoundBoots");
    effectsWithoutMagnitude.push_back("sEffectBoundShield");
    effectsWithoutMagnitude.push_back("sEffectBoundGloves");
    effectsWithoutMagnitude.push_back("sEffectStuntedMagicka");
    effectsWithoutMagnitude.push_back("sEffectMark");
    effectsWithoutMagnitude.push_back("sEffectRecall");
    effectsWithoutMagnitude.push_back("sEffectDivineIntervention");
    effectsWithoutMagnitude.push_back("sEffectAlmsiviIntervention");
    effectsWithoutMagnitude.push_back("sEffectCureCommonDisease");
    effectsWithoutMagnitude.push_back("sEffectCureBlightDisease");
    effectsWithoutMagnitude.push_back("sEffectCureCorprusDisease");
    effectsWithoutMagnitude.push_back("sEffectCurePoison");
    effectsWithoutMagnitude.push_back("sEffectCureParalyzation");
    effectsWithoutMagnitude.push_back("sEffectRemoveCurse");
    effectsWithoutMagnitude.push_back("sEffectSummonCreature01");
    effectsWithoutMagnitude.push_back("sEffectSummonCreature02");
    effectsWithoutMagnitude.push_back("sEffectSummonCreature03");
    effectsWithoutMagnitude.push_back("sEffectSummonCreature04");
    effectsWithoutMagnitude.push_back("sEffectSummonCreature05");
    effectsWithoutMagnitude.push_back("sEffectSummonFabricant");

    return (std::find(effectsWithoutMagnitude.begin(), effectsWithoutMagnitude.end(), effect) == effectsWithoutMagnitude.end());
}

bool MWSpellEffect::effectInvolvesAttribute (const std::string& effect)
{
    return (effect == "sEffectRestoreAttribute"
        || effect == "sEffectAbsorbAttribute"
        || effect == "sEffectDrainAttribute"
        || effect == "sEffectFortifyAttribute"
        || effect == "sEffectDamageAttribute");
}

bool MWSpellEffect::effectInvolvesSkill (const std::string& effect)
{
    return (effect == "sEffectRestoreSkill"
        || effect == "sEffectAbsorbSkill"
        || effect == "sEffectDrainSkill"
        || effect == "sEffectFortifySkill"
        || effect == "sEffectDamageSkill");
}

MWSpellEffect::~MWSpellEffect()
{
}

void MWSpellEffect::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mTextWidget, "Text");
    assignWidget(mImageWidget, "Image");
}

/* MWDynamicStat */

MWDynamicStat::MWDynamicStat()
: mValue(0)
, mMax(1)
, mTextWidget(nullptr)
, mBarWidget(nullptr)
, mBarTextWidget(nullptr)
{
}

void MWDynamicStat::setValue(int cur, int max)
{
    mValue = cur;
    mMax = max;

    if (mBarWidget)
    {
        mBarWidget->setProgressRange(mMax);
        mBarWidget->setProgressPosition(mValue);
    }


    if (mBarTextWidget)
    {
        if (mValue >= 0 && mMax > 0)
        {
            std::stringstream out;
            out << mValue << "/" << mMax;
            static_cast<MyGUI::TextBox*>(mBarTextWidget)->setCaption(out.str().c_str());
        }
        else
            static_cast<MyGUI::TextBox*>(mBarTextWidget)->setCaption("");
    }
}
void MWDynamicStat::setTitle(const std::string& text)
{
    if (mTextWidget)
        static_cast<MyGUI::TextBox*>(mTextWidget)->setCaption(text);
}

MWDynamicStat::~MWDynamicStat()
{
}

void MWDynamicStat::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mTextWidget, "Text");
    assignWidget(mBarWidget, "Bar");
    assignWidget(mBarTextWidget, "BarText");
}
