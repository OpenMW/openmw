#include "review.hpp"

#include <cmath>

#include <MyGUI_ScrollView.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/autocalcspell.hpp"

#include "tooltips.hpp"

namespace
{
    void adjustButtonSize(MyGUI::Button *button)
    {
        // adjust size of button to fit its text
        MyGUI::IntSize size = button->getTextSize();
        button->setSize(size.width + 24, button->getSize().height);
    }
}

namespace MWGui
{
    ReviewDialog::ReviewDialog()
        : WindowModal("openmw_chargen_review.layout"),
          mUpdateSkillArea(false)
    {
        // Centre dialog
        center();

        // Setup static stats
        MyGUI::Button* button;
        getWidget(mNameWidget, "NameText");
        getWidget(button, "NameButton");
        adjustButtonSize(button);
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onNameClicked);

        getWidget(mRaceWidget, "RaceText");
        getWidget(button, "RaceButton");
        adjustButtonSize(button);
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onRaceClicked);

        getWidget(mClassWidget, "ClassText");
        getWidget(button, "ClassButton");
        adjustButtonSize(button);
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onClassClicked);

        getWidget(mBirthSignWidget, "SignText");
        getWidget(button, "SignButton");
        adjustButtonSize(button);
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onBirthSignClicked);

        // Setup dynamic stats
        getWidget(mHealth, "Health");
        mHealth->setTitle(MWBase::Environment::get().getWindowManager()->getGameSettingString("sHealth", ""));
        mHealth->setValue(45, 45);

        getWidget(mMagicka, "Magicka");
        mMagicka->setTitle(MWBase::Environment::get().getWindowManager()->getGameSettingString("sMagic", ""));
        mMagicka->setValue(50, 50);

        getWidget(mFatigue, "Fatigue");
        mFatigue->setTitle(MWBase::Environment::get().getWindowManager()->getGameSettingString("sFatigue", ""));
        mFatigue->setValue(160, 160);

        // Setup attributes

        Widgets::MWAttributePtr attribute;
        for (int idx = 0; idx < ESM::Attribute::Length; ++idx)
        {
            getWidget(attribute, std::string("Attribute") + MyGUI::utility::toString(idx));
            mAttributeWidgets.insert(std::make_pair(static_cast<int>(ESM::Attribute::sAttributeIds[idx]), attribute));
            attribute->setAttributeId(ESM::Attribute::sAttributeIds[idx]);
            attribute->setAttributeValue(Widgets::MWAttribute::AttributeValue());
        }

        // Setup skills
        getWidget(mSkillView, "SkillView");
        mSkillView->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            mSkillValues.insert(std::make_pair(i, MWMechanics::SkillValue()));
            mSkillWidgetMap.insert(std::make_pair(i, static_cast<MyGUI::TextBox*> (0)));
        }

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onBackClicked);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onOkClicked);
    }

    void ReviewDialog::onOpen()
    {
        WindowModal::onOpen();
        mUpdateSkillArea = true;
    }

    void ReviewDialog::onFrame(float /*duration*/)
    {
        if (mUpdateSkillArea)
        {
            updateSkillArea();
            mUpdateSkillArea = false;
        }
    }

    void ReviewDialog::setPlayerName(const std::string &name)
    {
        mNameWidget->setCaption(name);
    }

    void ReviewDialog::setRace(const std::string &raceId)
    {
        mRaceId = raceId;

        const ESM::Race *race =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().search(mRaceId);
        if (race)
        {
            ToolTips::createRaceToolTip(mRaceWidget, race);
            mRaceWidget->setCaption(race->mName);
        }

        mUpdateSkillArea = true;
    }

    void ReviewDialog::setClass(const ESM::Class& class_)
    {
        mKlass = class_;
        mClassWidget->setCaption(mKlass.mName);
        ToolTips::createClassToolTip(mClassWidget, mKlass);
    }

    void ReviewDialog::setBirthSign(const std::string& signId)
    {
        mBirthSignId = signId;

        const ESM::BirthSign *sign =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().search(mBirthSignId);
        if (sign)
        {
            mBirthSignWidget->setCaption(sign->mName);
            ToolTips::createBirthsignToolTip(mBirthSignWidget, mBirthSignId);
        }

        mUpdateSkillArea = true;
    }

    void ReviewDialog::setHealth(const MWMechanics::DynamicStat<float>& value)
    {
        int current = std::max(0, static_cast<int>(value.getCurrent()));
        int modified = static_cast<int>(value.getModified());

        mHealth->setValue(current, modified);
        std::string valStr =  MyGUI::utility::toString(current) + " / " + MyGUI::utility::toString(modified);
        mHealth->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
    }

    void ReviewDialog::setMagicka(const MWMechanics::DynamicStat<float>& value)
    {
        int current = std::max(0, static_cast<int>(value.getCurrent()));
        int modified = static_cast<int>(value.getModified());

        mMagicka->setValue(current, modified);
        std::string valStr =  MyGUI::utility::toString(current) + " / " + MyGUI::utility::toString(modified);
        mMagicka->setUserString("Caption_HealthDescription", "#{sMagDesc}\n" + valStr);
    }

    void ReviewDialog::setFatigue(const MWMechanics::DynamicStat<float>& value)
    {
        int current = static_cast<int>(value.getCurrent());
        int modified = static_cast<int>(value.getModified());

        mFatigue->setValue(current, modified);
        std::string valStr =  MyGUI::utility::toString(current) + " / " + MyGUI::utility::toString(modified);
        mFatigue->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
    }

    void ReviewDialog::setAttribute(ESM::Attribute::AttributeID attributeId, const MWMechanics::AttributeValue& value)
    {
        std::map<int, Widgets::MWAttributePtr>::iterator attr = mAttributeWidgets.find(static_cast<int>(attributeId));
        if (attr == mAttributeWidgets.end())
            return;

        if (attr->second->getAttributeValue() != value)
        {
            attr->second->setAttributeValue(value);
            mUpdateSkillArea = true;
        }
    }

    void ReviewDialog::setSkillValue(ESM::Skill::SkillEnum skillId, const MWMechanics::SkillValue& value)
    {
        mSkillValues[skillId] = value;
        MyGUI::TextBox* widget = mSkillWidgetMap[skillId];
        if (widget)
        {
            float modified = static_cast<float>(value.getModified()), base = static_cast<float>(value.getBase());
            std::string text = MyGUI::utility::toString(std::floor(modified));
            std::string state = "normal";
            if (modified > base)
                state = "increased";
            else if (modified < base)
                state = "decreased";

            widget->setCaption(text);
            widget->_setWidgetState(state);
        }

        mUpdateSkillArea = true;
    }

    void ReviewDialog::configureSkills(const std::vector<int>& major, const std::vector<int>& minor)
    {
        mMajorSkills = major;
        mMinorSkills = minor;

        // Update misc skills with the remaining skills not in major or minor
        std::set<int> skillSet;
        std::copy(major.begin(), major.end(), std::inserter(skillSet, skillSet.begin()));
        std::copy(minor.begin(), minor.end(), std::inserter(skillSet, skillSet.begin()));
        mMiscSkills.clear();
        for (const int skill : ESM::Skill::sSkillIds)
        {
            if (skillSet.find(skill) == skillSet.end())
                mMiscSkills.push_back(skill);
        }

        mUpdateSkillArea = true;
    }

    void ReviewDialog::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::ImageBox* separator = mSkillView->createWidget<MyGUI::ImageBox>("MW_HLine", MyGUI::IntCoord(10, coord1.top, coord1.width + coord2.width - 4, 18), MyGUI::Align::Default);
        separator->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(separator);

        coord1.top += separator->getHeight();
        coord2.top += separator->getHeight();
    }

    void ReviewDialog::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* groupWidget = mSkillView->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, coord1.top, coord1.width + coord2.width, coord1.height), MyGUI::Align::Default);
        groupWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);
        groupWidget->setCaption(label);
        mSkillWidgets.push_back(groupWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;
    }

    MyGUI::TextBox* ReviewDialog::addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* skillNameWidget;
        MyGUI::TextBox* skillValueWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Default);
        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        skillValueWidget = mSkillView->createWidget<MyGUI::TextBox>("SandTextRight", coord2, MyGUI::Align::Default);
        skillValueWidget->setCaption(value);
        skillValueWidget->_setWidgetState(state);
        skillValueWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(skillNameWidget);
        mSkillWidgets.push_back(skillValueWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;

        return skillValueWidget;
    }

    void ReviewDialog::addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* skillNameWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(skillNameWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;
    }

    void ReviewDialog::addItem(const ESM::Spell* spell, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2)
    {
        Widgets::MWSpellPtr widget = mSkillView->createWidget<Widgets::MWSpell>("MW_StatName", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
        widget->setSpellId(spell->mId);
        widget->setUserString("ToolTipType", "Spell");
        widget->setUserString("Spell", spell->mId);
        widget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(widget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;
    }

    void ReviewDialog::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        // Add a line separator if there are items above
        if (!mSkillWidgets.empty())
        {
            addSeparator(coord1, coord2);
        }

        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString(titleId, titleDefault), coord1, coord2);

        for (const int& skillId : skills)
        {
            if (skillId < 0 || skillId >= ESM::Skill::Length) // Skip unknown skill indexes
                continue;
            assert(skillId >= 0 && skillId < ESM::Skill::Length);
            const std::string &skillNameId = ESM::Skill::sSkillNameIds[skillId];
            const MWMechanics::SkillValue &stat = mSkillValues.find(skillId)->second;
            int base = stat.getBase();
            int modified = stat.getModified();

            std::string state = "normal";
            if (modified > base)
                state = "increased";
            else if (modified < base)
                state = "decreased";
            MyGUI::TextBox* widget = addValueItem(MWBase::Environment::get().getWindowManager()->getGameSettingString(skillNameId, skillNameId), MyGUI::utility::toString(static_cast<int>(modified)), state, coord1, coord2);

            for (int i=0; i<2; ++i)
            {
                ToolTips::createSkillToolTip(mSkillWidgets[mSkillWidgets.size()-1-i], skillId);
            }

            mSkillWidgetMap[skillId] = widget;
        }
    }

    void ReviewDialog::updateSkillArea()
    {
        for (MyGUI::Widget* skillWidget : mSkillWidgets)
        {
            MyGUI::Gui::getInstance().destroyWidget(skillWidget);
        }
        mSkillWidgets.clear();

        const int valueSize = 40;
        MyGUI::IntCoord coord1(10, 0, mSkillView->getWidth() - (10 + valueSize) - 24, 18);
        MyGUI::IntCoord coord2(coord1.left + coord1.width, coord1.top, valueSize, coord1.height);

        if (!mMajorSkills.empty())
            addSkills(mMajorSkills, "sSkillClassMajor", "Major Skills", coord1, coord2);

        if (!mMinorSkills.empty())
            addSkills(mMinorSkills, "sSkillClassMinor", "Minor Skills", coord1, coord2);

        if (!mMiscSkills.empty())
            addSkills(mMiscSkills, "sSkillClassMisc", "Misc Skills", coord1, coord2);

        // starting spells
        std::vector<std::string> spells;

        const ESM::Race* race = nullptr;
        if (!mRaceId.empty())
            race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(mRaceId);

        int skills[ESM::Skill::Length];
        for (int i=0; i<ESM::Skill::Length; ++i)
            skills[i] = mSkillValues.find(i)->second.getBase();

        int attributes[ESM::Attribute::Length];
        for (int i=0; i<ESM::Attribute::Length; ++i)
            attributes[i] = mAttributeWidgets[i]->getAttributeValue().getBase();

        std::vector<std::string> selectedSpells = MWMechanics::autoCalcPlayerSpells(skills, attributes, race);
        for (std::string& spellId : selectedSpells)
        {
            std::string lower = Misc::StringUtils::lowerCase(spellId);
            if (std::find(spells.begin(), spells.end(), lower) == spells.end())
                spells.push_back(lower);
        }

        if (race)
        {
            for (const std::string& spellId : race->mPowers.mList)
            {
                std::string lower = Misc::StringUtils::lowerCase(spellId);
                if (std::find(spells.begin(), spells.end(), lower) == spells.end())
                    spells.push_back(lower);
            }
        }

        if (!mBirthSignId.empty())
        {
            const ESM::BirthSign* sign = MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().find(mBirthSignId);
            for (const std::string& spellId : sign->mPowers.mList)
            {
                std::string lower = Misc::StringUtils::lowerCase(spellId);
                if (std::find(spells.begin(), spells.end(), lower) == spells.end())
                    spells.push_back(lower);
            }
        }

        if (!mSkillWidgets.empty())
            addSeparator(coord1, coord2);
        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString("sTypeAbility", "Abilities"), coord1, coord2);
        for (std::string& spellId : spells)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
            if (spell->mData.mType == ESM::Spell::ST_Ability)
                addItem(spell, coord1, coord2);
        }

        addSeparator(coord1, coord2);
        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString("sTypePower", "Powers"), coord1, coord2);
        for (std::string& spellId : spells)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
            if (spell->mData.mType == ESM::Spell::ST_Power)
                addItem(spell, coord1, coord2);
        }

        addSeparator(coord1, coord2);
        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString("sTypeSpell", "Spells"), coord1, coord2);
        for (std::string& spellId : spells)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
            if (spell->mData.mType == ESM::Spell::ST_Spell)
                addItem(spell, coord1, coord2);
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSkillView->setVisibleVScroll(false);
        mSkillView->setCanvasSize (mSkillView->getWidth(), std::max(mSkillView->getHeight(), coord1.top));
        mSkillView->setVisibleVScroll(true);
    }

    // widget controls

    void ReviewDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        eventDone(this);
    }

    void ReviewDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    void ReviewDialog::onNameClicked(MyGUI::Widget* _sender)
    {
        eventActivateDialog(NAME_DIALOG);
    }

    void ReviewDialog::onRaceClicked(MyGUI::Widget* _sender)
    {
        eventActivateDialog(RACE_DIALOG);
    }

    void ReviewDialog::onClassClicked(MyGUI::Widget* _sender)
    {
        eventActivateDialog(CLASS_DIALOG);
    }

    void ReviewDialog::onBirthSignClicked(MyGUI::Widget* _sender)
    {
        eventActivateDialog(BIRTHSIGN_DIALOG);
    }

    void ReviewDialog::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSkillView->getViewOffset().top + _rel*0.3 > 0)
            mSkillView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSkillView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mSkillView->getViewOffset().top + _rel*0.3)));
    }

}
