#include "widgets.hpp"
#include "window_manager.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "components/esm_store/store.hpp"

#include <boost/lexical_cast.hpp>

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
            skillNameWidget->setCaption("");
        }
        else
        {
            const std::string &name = manager->getGameSettingString(ESM::Skill::sSkillNameIds[skillId], "");
            skillNameWidget->setCaption(name);
        }
    }
    if (skillValueWidget)
    {
        SkillValue::Type modified = value.getModified(), base = value.getBase();
        skillValueWidget->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            skillValueWidget->setState("increased");
        else if (modified < base)
            skillValueWidget->setState("decreased");
        else
            skillValueWidget->setState("normal");
    }
}

void MWSkill::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

void MWSkill::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
{
	Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

	initialiseWidgetSkin(_info);
}

MWSkill::~MWSkill()
{
	shutdownWidgetSkin();
}

void MWSkill::baseChangeWidgetSkin(ResourceSkin* _info)
{
	shutdownWidgetSkin();
	Base::baseChangeWidgetSkin(_info);
	initialiseWidgetSkin(_info);
}

void MWSkill::initialiseWidgetSkin(ResourceSkin* _info)
{
    for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
	{
        const std::string &name = *(*iter)->_getInternalData<std::string>();
		if (name == "StatName")
		{
			MYGUI_DEBUG_ASSERT( ! skillNameWidget, "widget already assigned");
			skillNameWidget = (*iter)->castType<StaticText>();
		}
		else if (name == "StatValue")
		{
			MYGUI_DEBUG_ASSERT( ! skillValueWidget, "widget already assigned");
			skillValueWidget = (*iter)->castType<StaticText>();
		}
		else if (name == "StatNameButton")
		{
			MYGUI_DEBUG_ASSERT( ! skillNameWidget, "widget already assigned");
            MyGUI::ButtonPtr button = (*iter)->castType<Button>();
            skillNameWidget = button;
            button->eventMouseButtonClick = MyGUI::newDelegate(this, &MWSkill::onClicked);
		}
		else if (name == "StatValueButton")
		{
			MYGUI_DEBUG_ASSERT( ! skillValueWidget, "widget already assigned");
            MyGUI::ButtonPtr button = (*iter)->castType<Button>();
            skillNameWidget = button;
            button->eventMouseButtonClick = MyGUI::newDelegate(this, &MWSkill::onClicked);
		}
	}
}

void MWSkill::shutdownWidgetSkin()
{
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
            attributeNameWidget->setCaption("");
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
            attributeNameWidget->setCaption(name);
        }
    }
    if (attributeValueWidget)
    {
        AttributeValue::Type modified = value.getModified(), base = value.getBase();
        attributeValueWidget->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            attributeValueWidget->setState("increased");
        else if (modified < base)
            attributeValueWidget->setState("decreased");
        else
            attributeValueWidget->setState("normal");
    }
}

void MWAttribute::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
{
	Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

	initialiseWidgetSkin(_info);
}

MWAttribute::~MWAttribute()
{
	shutdownWidgetSkin();
}

void MWAttribute::baseChangeWidgetSkin(ResourceSkin* _info)
{
	shutdownWidgetSkin();
	Base::baseChangeWidgetSkin(_info);
	initialiseWidgetSkin(_info);
}

void MWAttribute::initialiseWidgetSkin(ResourceSkin* _info)
{
    for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
	{
        const std::string &name = *(*iter)->_getInternalData<std::string>();
		if (name == "StatName")
		{
			MYGUI_DEBUG_ASSERT( ! attributeNameWidget, "widget already assigned");
			attributeNameWidget = (*iter)->castType<StaticText>();
		}
		else if (name == "StatValue")
		{
			MYGUI_DEBUG_ASSERT( ! attributeValueWidget, "widget already assigned");
			attributeValueWidget = (*iter)->castType<StaticText>();
		}
		else if (name == "StatNameButton")
		{
			MYGUI_DEBUG_ASSERT( ! attributeNameWidget, "widget already assigned");
            MyGUI::ButtonPtr button = (*iter)->castType<Button>();
            attributeNameWidget = button;
            button->eventMouseButtonClick = MyGUI::newDelegate(this, &MWAttribute::onClicked);
		}
		else if (name == "StatValue")
		{
			MYGUI_DEBUG_ASSERT( ! attributeValueWidget, "widget already assigned");
            MyGUI::ButtonPtr button = (*iter)->castType<Button>();
            attributeNameWidget = button;
            button->eventMouseButtonClick = MyGUI::newDelegate(this, &MWAttribute::onClicked);
		}
	}
}

void MWAttribute::shutdownWidgetSkin()
{
}

/* MWSpell */

MWSpell::MWSpell()
    : env(nullptr)
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
    ESMS::ESMStore &store = env->mWorld->getStore();
    const ESM::Spell *spell = store.spells.search(id);
    MYGUI_ASSERT(spell, "spell with id '" << id << "' not found");

    MWSpellEffectPtr effect = nullptr;
    std::vector<ESM::ENAMstruct>::const_iterator end = spell->effects.list.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->effects.list.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setEnvironment(env);
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        coord.top += effect->getHeight();
    }
}

void MWSpell::updateWidgets()
{
    if (spellNameWidget && env)
    {
        ESMS::ESMStore &store = env->mWorld->getStore();
        const ESM::Spell *spell = store.spells.search(id);
        if (spell)
            spellNameWidget->setCaption(spell->name);
        else
            spellNameWidget->setCaption("");
    }
}

void MWSpell::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
{
	Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

	initialiseWidgetSkin(_info);
}

MWSpell::~MWSpell()
{
	shutdownWidgetSkin();
}

void MWSpell::baseChangeWidgetSkin(ResourceSkin* _info)
{
	shutdownWidgetSkin();
	Base::baseChangeWidgetSkin(_info);
	initialiseWidgetSkin(_info);
}

void MWSpell::initialiseWidgetSkin(ResourceSkin* _info)
{
    for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
	{
        const std::string &name = *(*iter)->_getInternalData<std::string>();
		if (name == "StatName")
		{
			MYGUI_DEBUG_ASSERT( ! spellNameWidget, "widget already assigned");
			spellNameWidget = (*iter)->castType<StaticText>();
		}
	}
}

void MWSpell::shutdownWidgetSkin()
{
}

/* MWSpellEffect */

MWSpellEffect::MWSpellEffect()
    : env(nullptr)
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
    if (!env)
        return;

    ESMS::ESMStore &store = env->mWorld->getStore();
    WindowManager *wm = env->mWindowManager;
    const ESM::MagicEffect *magicEffect = store.magicEffects.search(effect.effectID);
    if (textWidget)
    {
        if (magicEffect)
        {
            // TODO: Get name of effect from GMST
            std::string spellLine = "";
            if (effect.skill >= 0 && effect.skill < ESM::Skill::Length)
            {
                spellLine += " " + wm->getGameSettingString(ESM::Skill::sSkillNameIds[effect.skill], "");
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
                spellLine += " " + wm->getGameSettingString(attributes[effect.attribute], "");
            }
            if (effect.magnMin >= 0 || effect.magnMax >= 0)
            {
                if (effect.magnMin == effect.magnMax)
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + " pts";
                else
                {
                    spellLine += " " + boost::lexical_cast<std::string>(effect.magnMin) + " to " + boost::lexical_cast<std::string>(effect.magnMin) + " pts";
                }
            }
            if (effect.duration >= 0)
            {
                spellLine += " for " + boost::lexical_cast<std::string>(effect.duration) + " secs";
            }
            if (effect.range == ESM::RT_Self)
                spellLine += " on Self";
            else if (effect.range == ESM::RT_Touch)
                spellLine += " on Touch";
            else if (effect.range == ESM::RT_Target)
                spellLine += " on Target";
            textWidget->setCaption(spellLine);
        }
        else
            textWidget->setCaption("");
    }
    if (imageWidget)
    {
        std::string path = std::string("icons\\") + magicEffect->icon;
        fixTexturePath(path);
        imageWidget->setImageTexture(path);
    }
}

void MWSpellEffect::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
{
	Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

	initialiseWidgetSkin(_info);
}

MWSpellEffect::~MWSpellEffect()
{
	shutdownWidgetSkin();
}

void MWSpellEffect::baseChangeWidgetSkin(ResourceSkin* _info)
{
	shutdownWidgetSkin();
	Base::baseChangeWidgetSkin(_info);
	initialiseWidgetSkin(_info);
}

void MWSpellEffect::initialiseWidgetSkin(ResourceSkin* _info)
{
    for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
	{
        const std::string &name = *(*iter)->_getInternalData<std::string>();
		if (name == "Text")
		{
			MYGUI_DEBUG_ASSERT( ! textWidget, "widget already assigned");
			textWidget = (*iter)->castType<StaticText>();
		}
		else if (name == "Image")
		{
			MYGUI_DEBUG_ASSERT( ! imageWidget, "widget already assigned");
			imageWidget = (*iter)->castType<StaticImage>();
		}
	}
}

void MWSpellEffect::shutdownWidgetSkin()
{
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
            barTextWidget->setCaption(out.str().c_str());
        }
        else
            barTextWidget->setCaption("");
    }
}
void MWDynamicStat::setTitle(const std::string text)
{
    if (textWidget)
        textWidget->setCaption(text);
}

void MWDynamicStat::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
{
    Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

    initialiseWidgetSkin(_info);
}

MWDynamicStat::~MWDynamicStat()
{
    shutdownWidgetSkin();
}

void MWDynamicStat::baseChangeWidgetSkin(ResourceSkin* _info)
{
    shutdownWidgetSkin();
    Base::baseChangeWidgetSkin(_info);
    initialiseWidgetSkin(_info);
}

void MWDynamicStat::initialiseWidgetSkin(ResourceSkin* _info)
{
    for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
    {
        const std::string &name = *(*iter)->_getInternalData<std::string>();
        if (name == "Text")
        {
            MYGUI_DEBUG_ASSERT( ! textWidget, "widget already assigned");
            textWidget = (*iter)->castType<StaticText>();
        }
        else if (name == "Bar")
        {
            MYGUI_DEBUG_ASSERT( ! barWidget, "widget already assigned");
            barWidget = (*iter)->castType<Progress>();
        }
        else if (name == "BarText")
        {
            MYGUI_DEBUG_ASSERT( ! barTextWidget, "widget already assigned");
            barTextWidget = (*iter)->castType<StaticText>();
        }
    }
}

void MWDynamicStat::shutdownWidgetSkin()
{
}
