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

void MWSpell::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord)
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
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        coord.top += effect->getHeight();
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

/* MWEnchantment */

MWEnchantment::MWEnchantment()
    : mWindowManager(nullptr)
{
}

void MWEnchantment::setEnchantmentId(const std::string &enchantId)
{
    id = enchantId;
    updateWidgets();
}

void MWEnchantment::createEffectWidgets(std::vector<MyGUI::WidgetPtr> &effects, MyGUI::WidgetPtr creator, MyGUI::IntCoord &coord)
{
    const ESMS::ESMStore &store = mWindowManager->getStore();
    const ESM::Enchantment *enchant = store.enchants.search(id);
    MYGUI_ASSERT(enchant, "enchantment with id '" << id << "' not found");

    MWSpellEffectPtr effect = nullptr;
    std::vector<ESM::ENAMstruct>::const_iterator end = enchant->effects.list.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = enchant->effects.list.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        coord.top += effect->getHeight();
    }
}

void MWEnchantment::updateWidgets()
{
}

void MWEnchantment::initialiseOverride()
{
    Base::initialiseOverride();
}

MWEnchantment::~MWEnchantment()
{
}

/* MWSpellEffect */

MWSpellEffect::MWSpellEffect()
    : mWindowManager(nullptr)
    , imageWidget(nullptr)
    , textWidget(nullptr)
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

            // TODO: Get name of effect from GMST
            std::string spellLine = "";
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
            if (effect.magnMin >= 0 || effect.magnMax >= 0)
            {
                if (effect.magnMin == effect.magnMax)
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + " " + ((effect.magnMin == 1) ? pt : pts);
                else
                {
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + to + boost::lexical_cast<std::string>(effect.magnMin) + " " + pts;
                }
            }
            if (effect.duration >= 0)
            {
                spellLine += " " + mWindowManager->getGameSettingString("sfor", "") + " " + boost::lexical_cast<std::string>(effect.duration) + ((effect.duration == 1) ? sec : secs);
            }

            std::string on = mWindowManager->getGameSettingString("sonword", "");
            if (effect.range == ESM::RT_Self)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeSelf", "");
            else if (effect.range == ESM::RT_Touch)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTouch", "");
            else if (effect.range == ESM::RT_Target)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTarget", "");
            static_cast<MyGUI::TextBox*>(textWidget)->setCaption(spellLine);
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
