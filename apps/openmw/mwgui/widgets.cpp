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
    : manager(nullptr)
    , skillId(ESM::Skill::Length)
    , skillNameWidget(nullptr)
    , skillValueWidget(nullptr)
{
}

void MWSkill::setSkillId(ESM::Skill::SkillEnum skill)
{
    skillId = skill;
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

void MWSkill::setSkillValue(const SkillValue& value_)
{
    value = value_;
    updateWidgets();
}

void MWSkill::updateWidgets()
{
    if (skillNameWidget && manager)
    {
        if (skillId == ESM::Skill::Length)
        {
            static_cast<MyGUI::TextBox*>(skillNameWidget)->setCaption("");
        }
        else
        {
            const std::string &name = manager->getGameSettingString(ESM::Skill::sSkillNameIds[skillId], "");
            static_cast<MyGUI::TextBox*>(skillNameWidget)->setCaption(name);
        }
    }
    if (skillValueWidget)
    {
        SkillValue::Type modified = value.getModified(), base = value.getBase();
        static_cast<MyGUI::TextBox*>(skillValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            skillValueWidget->_setWidgetState("increased");
        else if (modified < base)
            skillValueWidget->_setWidgetState("decreased");
        else
            skillValueWidget->_setWidgetState("normal");
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

    assignWidget(skillNameWidget, "StatName");
    assignWidget(skillValueWidget, "StatValue");

    MyGUI::ButtonPtr button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        skillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        skillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }
}

/* MWAttribute */

MWAttribute::MWAttribute()
    : manager(nullptr)
    , id(-1)
    , attributeNameWidget(nullptr)
    , attributeValueWidget(nullptr)
{
}

void MWAttribute::setAttributeId(int attributeId)
{
    id = attributeId;
    updateWidgets();
}

void MWAttribute::setAttributeValue(const AttributeValue& value_)
{
    value = value_;
    updateWidgets();
}

void MWAttribute::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

void MWAttribute::updateWidgets()
{
    if (attributeNameWidget && manager)
    {
        if (id < 0 || id >= 8)
        {
            static_cast<MyGUI::TextBox*>(attributeNameWidget)->setCaption("");
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
            const std::string &name = manager->getGameSettingString(attributes[id], "");
            static_cast<MyGUI::TextBox*>(attributeNameWidget)->setCaption(name);
        }
    }
    if (attributeValueWidget)
    {
        AttributeValue::Type modified = value.getModified(), base = value.getBase();
        static_cast<MyGUI::TextBox*>(attributeValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            attributeValueWidget->_setWidgetState("increased");
        else if (modified < base)
            attributeValueWidget->_setWidgetState("decreased");
        else
            attributeValueWidget->_setWidgetState("normal");
    }
}

MWAttribute::~MWAttribute()
{
}

void MWAttribute::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(attributeNameWidget, "StatName");
    assignWidget(attributeValueWidget, "StatValue");
    
    MyGUI::ButtonPtr button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        attributeNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        attributeValueWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }
}

/* MWSpell */

MWSpell::MWSpell()
    : mWindowManager(nullptr)
    , spellNameWidget(nullptr)
{
}

void MWSpell::setSpellId(const std::string &spellId)
{
    id = spellId;
    updateWidgets();
}

void MWSpell::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, int flags)
{
    const ESMS::ESMStore &store = mWindowManager->getStore();
    const ESM::Spell *spell = store.spells.search(id);
    MYGUI_ASSERT(spell, "spell with id '" << id << "' not found");

    MWSpellEffectPtr effect = nullptr;
    std::vector<ESM::ENAMstruct>::const_iterator end = spell->effects.list.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->effects.list.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        effect->setFlags(flags);
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        coord.top += effect->getHeight();
        coord.width = std::max(coord.width, effect->getRequestedWidth());
    }
}

void MWSpell::updateWidgets()
{
    if (spellNameWidget && mWindowManager)
    {
        const ESMS::ESMStore &store = mWindowManager->getStore();
        const ESM::Spell *spell = store.spells.search(id);
        if (spell)
            static_cast<MyGUI::TextBox*>(spellNameWidget)->setCaption(spell->name);
        else
            static_cast<MyGUI::TextBox*>(spellNameWidget)->setCaption("");
    }
}

void MWSpell::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(spellNameWidget, "StatName");
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

void MWEffectList::setEffectList(const ESM::EffectList* list)
{
    mEffectList = list;
    updateWidgets();
}

void MWEffectList::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord, bool center, int flags)
{
    // We don't know the width of all the elements beforehand, so we do it in
    // 2 steps: first, create all widgets and check their width
    MWSpellEffectPtr effect = nullptr;
    std::vector<ESM::ENAMstruct>::const_iterator end = mEffectList->list.end();
    int maxwidth = coord.width;
    for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffectList->list.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        effect->setFlags(flags);
        effect->setSpellEffect(*it);
        effects.push_back(effect);

        if (effect->getRequestedWidth() > maxwidth)
            maxwidth = effect->getRequestedWidth();

        coord.top += effect->getHeight();
    }

    // then adjust the size for all widgets
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

/* MWSpellEffect */

MWSpellEffect::MWSpellEffect()
    : mWindowManager(nullptr)
    , imageWidget(nullptr)
    , textWidget(nullptr)
    , mRequestedWidth(0)
    , mFlags(0)
{
}

void MWSpellEffect::setSpellEffect(SpellEffectValue value)
{
    effect = value;
    updateWidgets();
}

void MWSpellEffect::updateWidgets()
{
    if (!mWindowManager)
        return;

    // lists effects that have no magnitude (e.g. invisiblity)
    /// \todo this list is probably incomplete
    std::vector<std::string> effectsWithoutMagnitude;
    effectsWithoutMagnitude.push_back("sEffectInvisibility");
    effectsWithoutMagnitude.push_back("sEffectStuntedMagicka");
    effectsWithoutMagnitude.push_back("sEffectParalyze");

    // lists effects that have no duration (e.g. open lock)
    /// \todo this list is probably incomplete
    std::vector<std::string> effectsWithoutDuration;
    effectsWithoutDuration.push_back("sEffectOpen");

    const ESMS::ESMStore &store = mWindowManager->getStore();
    const ESM::MagicEffect *magicEffect = store.magicEffects.search(effect.effectID);
    if (textWidget)
    {
        if (magicEffect)
        {
            std::string pt =  mWindowManager->getGameSettingString("spoint", "");
            std::string pts =  mWindowManager->getGameSettingString("spoints", "");
            std::string to =  " " + mWindowManager->getGameSettingString("sTo", "") + " ";
            std::string sec =  " " + mWindowManager->getGameSettingString("ssecond", "");
            std::string secs =  " " + mWindowManager->getGameSettingString("sseconds", "");

            std::string effectIDStr = effectIDToString(effect.effectID);
            std::string spellLine = mWindowManager->getGameSettingString(effectIDStr, "");
            if (effect.skill >= 0 && effect.skill < ESM::Skill::Length)
            {
                spellLine += " " + mWindowManager->getGameSettingString(ESM::Skill::sSkillNameIds[effect.skill], "");
            }
            if (effect.attribute >= 0 && effect.attribute < 8)
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
                spellLine += " " + mWindowManager->getGameSettingString(attributes[effect.attribute], "");
            }

            bool hasMagnitude = (std::find(effectsWithoutMagnitude.begin(), effectsWithoutMagnitude.end(), effectIDStr) == effectsWithoutMagnitude.end());
            if ((effect.magnMin >= 0 || effect.magnMax >= 0) && hasMagnitude)
            {
                if (effect.magnMin == effect.magnMax)
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + " " + ((effect.magnMin == 1) ? pt : pts);
                else
                {
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + to + boost::lexical_cast<std::string>(effect.magnMax) + " " + pts;
                }
            }

            // constant effects have no duration and no target
            if (!(mFlags & MWEffectList::EF_Constant))
            {
                bool hasDuration = (std::find(effectsWithoutDuration.begin(), effectsWithoutDuration.end(), effectIDStr) == effectsWithoutDuration.end());
                if (effect.duration >= 0 && hasDuration)
                {
                    spellLine += " " + mWindowManager->getGameSettingString("sfor", "") + " " + boost::lexical_cast<std::string>(effect.duration) + ((effect.duration == 1) ? sec : secs);
                }

                // potions have no target
                if (!(mFlags & MWEffectList::EF_Potion))
                {
                    std::string on = mWindowManager->getGameSettingString("sonword", "");
                    if (effect.range == ESM::RT_Self)
                        spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeSelf", "");
                    else if (effect.range == ESM::RT_Touch)
                        spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTouch", "");
                    else if (effect.range == ESM::RT_Target)
                        spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTarget", "");
                }
            }

            static_cast<MyGUI::TextBox*>(textWidget)->setCaption(spellLine);
            mRequestedWidth = textWidget->getTextSize().width + 24;
        }
        else
            static_cast<MyGUI::TextBox*>(textWidget)->setCaption("");
    }
    if (imageWidget)
    {
        std::string path = std::string("icons\\") + magicEffect->icon;
        fixTexturePath(path);
        imageWidget->setImageTexture(path);
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

    assert(names.find(effectID) != names.end() && "Unimplemented effect type");

    return names[effectID];
}

MWSpellEffect::~MWSpellEffect()
{
}

void MWSpellEffect::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(textWidget, "Text");
    assignWidget(imageWidget, "Image");
}

/* MWDynamicStat */

MWDynamicStat::MWDynamicStat()
: value(0)
, max(1)
, textWidget(nullptr)
, barWidget(nullptr)
, barTextWidget(nullptr)
{
}

void MWDynamicStat::setValue(int cur, int max_)
{
    value = cur;
    max = max_;

    if (barWidget)
    {
        barWidget->setProgressRange(max);
        barWidget->setProgressPosition(value);
    }


    if (barTextWidget)
    {
        if (value >= 0 && max > 0)
        {
            std::stringstream out;
            out << value << "/" << max;
            static_cast<MyGUI::TextBox*>(barTextWidget)->setCaption(out.str().c_str());
        }
        else
            static_cast<MyGUI::TextBox*>(barTextWidget)->setCaption("");
    }
}
void MWDynamicStat::setTitle(const std::string text)
{
    if (textWidget)
        static_cast<MyGUI::TextBox*>(textWidget)->setCaption(text);
}

MWDynamicStat::~MWDynamicStat()
{
}

void MWDynamicStat::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(textWidget, "Text");
    assignWidget(barWidget, "Bar");
    assignWidget(barTextWidget, "BarText");
}
