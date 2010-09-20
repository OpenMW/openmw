#include "widgets.hpp"
#include "window_manager.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "components/esm_store/store.hpp"

//#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace MWGui::Widgets;

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
