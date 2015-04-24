#include "review.hpp"

#include <MyGUI_ScrollView.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwworld/esmstore.hpp"

#include "tooltips.hpp"

#undef min
#undef max

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

    const int ReviewDialog::sLineHeight = 18;

    ReviewDialog::ReviewDialog()
        : WindowModal("openmw_chargen_review.layout")
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

    void ReviewDialog::open()
    {
        WindowModal::open();
        updateSkillArea();
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
    }

    void ReviewDialog::setHealth(const MWMechanics::DynamicStat<float>& value)
    {
        mHealth->setValue(static_cast<int>(value.getCurrent()), static_cast<int>(value.getModified()));
        std::string valStr =  MyGUI::utility::toString(value.getCurrent()) + "/" + MyGUI::utility::toString(value.getModified());
        mHealth->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
    }

    void ReviewDialog::setMagicka(const MWMechanics::DynamicStat<float>& value)
    {
        mMagicka->setValue(static_cast<int>(value.getCurrent()), static_cast<int>(value.getModified()));
        std::string valStr =  MyGUI::utility::toString(value.getCurrent()) + "/" + MyGUI::utility::toString(value.getModified());
        mMagicka->setUserString("Caption_HealthDescription", "#{sIntDesc}\n" + valStr);
    }

    void ReviewDialog::setFatigue(const MWMechanics::DynamicStat<float>& value)
    {
        mFatigue->setValue(static_cast<int>(value.getCurrent()), static_cast<int>(value.getModified()));
        std::string valStr =  MyGUI::utility::toString(value.getCurrent()) + "/" + MyGUI::utility::toString(value.getModified());
        mFatigue->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
    }

    void ReviewDialog::setAttribute(ESM::Attribute::AttributeID attributeId, const MWMechanics::AttributeValue& value)
    {
        std::map<int, Widgets::MWAttributePtr>::iterator attr = mAttributeWidgets.find(static_cast<int>(attributeId));
        if (attr == mAttributeWidgets.end())
            return;

        attr->second->setAttributeValue(value);
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

    }

    void ReviewDialog::configureSkills(const std::vector<int>& major, const std::vector<int>& minor)
    {
        mMajorSkills = major;
        mMinorSkills = minor;

        // Update misc skills with the remaining skills not in major or minor
        std::set<int> skillSet;
        std::copy(major.begin(), major.end(), std::inserter(skillSet, skillSet.begin()));
        std::copy(minor.begin(), minor.end(), std::inserter(skillSet, skillSet.begin()));
        boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator end = ESM::Skill::sSkillIds.end();
        mMiscSkills.clear();
        for (boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator it = ESM::Skill::sSkillIds.begin(); it != end; ++it)
        {
            int skill = *it;
            if (skillSet.find(skill) == skillSet.end())
                mMiscSkills.push_back(skill);
        }

        updateSkillArea();
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

        coord1.top += sLineHeight;
        coord2.top += sLineHeight;
    }

    MyGUI::TextBox* ReviewDialog::addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* skillNameWidget;
        MyGUI::TextBox* skillValueWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Default);
        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        skillValueWidget = mSkillView->createWidget<MyGUI::TextBox>("SandTextRight", coord2, MyGUI::Align::Top | MyGUI::Align::Right);
        skillValueWidget->setCaption(value);
        skillValueWidget->_setWidgetState(state);
        skillValueWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(skillNameWidget);
        mSkillWidgets.push_back(skillValueWidget);

        coord1.top += sLineHeight;
        coord2.top += sLineHeight;

        return skillValueWidget;
    }

    void ReviewDialog::addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* skillNameWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &ReviewDialog::onMouseWheel);

        mSkillWidgets.push_back(skillNameWidget);

        coord1.top += sLineHeight;
        coord2.top += sLineHeight;
    }

    void ReviewDialog::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        // Add a line separator if there are items above
        if (!mSkillWidgets.empty())
        {
            addSeparator(coord1, coord2);
        }

        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString(titleId, titleDefault), coord1, coord2);

        SkillList::const_iterator end = skills.end();
        for (SkillList::const_iterator it = skills.begin(); it != end; ++it)
        {
            int skillId = *it;
            if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
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
        for (std::vector<MyGUI::Widget*>::iterator it = mSkillWidgets.begin(); it != mSkillWidgets.end(); ++it)
        {
            MyGUI::Gui::getInstance().destroyWidget(*it);
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
