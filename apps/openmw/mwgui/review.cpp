#include "review.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <cmath>

using namespace MWGui;
using namespace Widgets;

const int ReviewDialog::lineHeight = 18;

ReviewDialog::ReviewDialog(MWWorld::Environment& environment)
    : WindowBase("openmw_chargen_review_layout.xml", environment)
    , lastPos(0)
{
    // Centre dialog
    center();

    WindowManager *wm = environment.mWindowManager;

    // Setup static stats
    ButtonPtr button;
    getWidget(nameWidget, "NameText");
    getWidget(button, "NameButton");
    button->setCaption(wm->getGameSettingString("sName", ""));
    button->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onNameClicked);;

    getWidget(raceWidget, "RaceText");
    getWidget(button, "RaceButton");
    button->setCaption(wm->getGameSettingString("sRace", ""));
    button->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onRaceClicked);;

    getWidget(classWidget, "ClassText");
    getWidget(button, "ClassButton");
    button->setCaption(wm->getGameSettingString("sClass", ""));
    button->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onClassClicked);;

    getWidget(birthSignWidget, "SignText");
    getWidget(button, "SignButton");
    button->setCaption(wm->getGameSettingString("sBirthSign", ""));
    button->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onBirthSignClicked);;

    // Setup dynamic stats
    getWidget(health, "Health");
    health->setTitle(wm->getGameSettingString("sHealth", ""));
    health->setValue(45, 45);

    getWidget(magicka, "Magicka");
    magicka->setTitle(wm->getGameSettingString("sMagic", ""));
    magicka->setValue(50, 50);

    getWidget(fatigue, "Fatigue");
    fatigue->setTitle(wm->getGameSettingString("sFatigue", ""));
    fatigue->setValue(160, 160);

    // Setup attributes

    MWAttributePtr attribute;
    for (int idx = 0; idx < ESM::Attribute::Length; ++idx)
    {
        getWidget(attribute, std::string("Attribute") + boost::lexical_cast<std::string>(idx));
        attributeWidgets.insert(std::make_pair(static_cast<int>(ESM::Attribute::attributeIds[idx]), attribute));
        attribute->setWindowManager(wm);
        attribute->setAttributeId(ESM::Attribute::attributeIds[idx]);
        attribute->setAttributeValue(MWAttribute::AttributeValue(0, 0));
    }

    // Setup skills
    getWidget(skillAreaWidget, "Skills");
    getWidget(skillClientWidget, "SkillClient");
    getWidget(skillScrollerWidget, "SkillScroller");

    skillScrollerWidget->eventScrollChangePosition = MyGUI::newDelegate(this, &ReviewDialog::onScrollChangePosition);
    updateScroller();

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        skillValues.insert(std::make_pair(i, MWMechanics::Stat<float>()));
        skillWidgetMap.insert(std::make_pair(i, static_cast<MyGUI::StaticText*> (0)));
    }

    static_cast<MyGUI::WindowPtr>(mMainWidget)->eventWindowChangeCoord = MyGUI::newDelegate(this, &ReviewDialog::onWindowResize);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onOkClicked);
}

void ReviewDialog::open()
{
    updateSkillArea();
    setVisible(true);
}

void ReviewDialog::onScrollChangePosition(MyGUI::VScrollPtr scroller, size_t pos)
{
    int diff = lastPos - pos;
    // Adjust position of all widget according to difference
    if (diff == 0)
        return;
    lastPos = pos;

    std::vector<MyGUI::WidgetPtr>::const_iterator end = skillWidgets.end();
    for (std::vector<MyGUI::WidgetPtr>::const_iterator it = skillWidgets.begin(); it != end; ++it)
    {
        (*it)->setCoord((*it)->getCoord() + MyGUI::IntPoint(0, diff));
    }
}

void ReviewDialog::onWindowResize(MyGUI::WidgetPtr window)
{
    updateScroller();
}

void ReviewDialog::setPlayerName(const std::string &name)
{
    nameWidget->setCaption(name);
}

void ReviewDialog::setRace(const std::string &raceId_)
{
    raceId = raceId_;
    const ESM::Race *race = environment.mWorld->getStore().races.search(raceId);
    if (race)
        raceWidget->setCaption(race->name);
}

void ReviewDialog::setClass(const ESM::Class& class_)
{
    klass = class_;
    classWidget->setCaption(klass.name);
}

void ReviewDialog::setBirthSign(const std::string& signId)
{
    birthSignId = signId;
    const ESM::BirthSign *sign = environment.mWorld->getStore().birthSigns.search(birthSignId);
    if (sign)
        birthSignWidget->setCaption(sign->name);
}

void ReviewDialog::setHealth(const MWMechanics::DynamicStat<int>& value)
{
    health->setValue(value.getCurrent(), value.getModified());
}

void ReviewDialog::setMagicka(const MWMechanics::DynamicStat<int>& value)
{
    magicka->setValue(value.getCurrent(), value.getModified());
}

void ReviewDialog::setFatigue(const MWMechanics::DynamicStat<int>& value)
{
    fatigue->setValue(value.getCurrent(), value.getModified());
}

void ReviewDialog::setAttribute(ESM::Attribute::AttributeID attributeId, const MWMechanics::Stat<int>& value)
{
    std::map<int, MWAttributePtr>::iterator attr = attributeWidgets.find(static_cast<int>(attributeId));
    if (attr == attributeWidgets.end())
        return;

    attr->second->setAttributeValue(value);
}

void ReviewDialog::setSkillValue(ESM::Skill::SkillEnum skillId, const MWMechanics::Stat<float>& value)
{
    skillValues[skillId] = value;
    MyGUI::StaticTextPtr widget = skillWidgetMap[skillId];
    if (widget)
    {
        float modified = value.getModified(), base = value.getBase();
        std::string text = boost::lexical_cast<std::string>(std::floor(modified));
        ColorStyle style = CS_Normal;
        if (modified > base)
            style = CS_Super;
        else if (modified < base)
            style = CS_Sub;

        setStyledText(widget, style, text);
    }
}

void ReviewDialog::configureSkills(const std::vector<int>& major, const std::vector<int>& minor)
{
    majorSkills = major;
    minorSkills = minor;

    // Update misc skills with the remaining skills not in major or minor
    std::set<int> skillSet;
    std::copy(major.begin(), major.end(), std::inserter(skillSet, skillSet.begin()));
    std::copy(minor.begin(), minor.end(), std::inserter(skillSet, skillSet.begin()));
    boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator end = ESM::Skill::skillIds.end();
    miscSkills.clear();
    for (boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator it = ESM::Skill::skillIds.begin(); it != end; ++it)
    {
        int skill = *it;
        if (skillSet.find(skill) == skillSet.end())
            miscSkills.push_back(skill);
    }
}

void ReviewDialog::setStyledText(MyGUI::StaticTextPtr widget, ColorStyle style, const std::string &value)
{
    widget->setCaption(value);
    if (style == CS_Super)
        widget->setTextColour(MyGUI::Colour(0, 1, 0));
    else if (style == CS_Sub)
        widget->setTextColour(MyGUI::Colour(1, 0, 0));
    else
        widget->setTextColour(MyGUI::Colour(1, 1, 1));
}

void ReviewDialog::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticImagePtr separator = skillClientWidget->createWidget<MyGUI::StaticImage>("MW_HLine", MyGUI::IntCoord(10, coord1.top, coord1.width + coord2.width - 4, 18), MyGUI::Align::Default);
    skillWidgets.push_back(separator);

    coord1.top += separator->getHeight();
    coord2.top += separator->getHeight();
}

void ReviewDialog::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr groupWidget = skillClientWidget->createWidget<MyGUI::StaticText>("SandBrightText", MyGUI::IntCoord(0, coord1.top, coord1.width + coord2.width, coord1.height), MyGUI::Align::Default);
    groupWidget->setCaption(label);
    skillWidgets.push_back(groupWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

MyGUI::StaticTextPtr ReviewDialog::addValueItem(const std::string text, const std::string &value, ColorStyle style, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr skillNameWidget, skillValueWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::StaticText>("SandText", coord1, MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillValueWidget = skillClientWidget->createWidget<MyGUI::StaticText>("SandTextRight", coord2, MyGUI::Align::Default);
    setStyledText(skillValueWidget, style, value);

    skillWidgets.push_back(skillNameWidget);
    skillWidgets.push_back(skillValueWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    return skillValueWidget;
}

void ReviewDialog::addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr skillNameWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::StaticText>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillWidgets.push_back(skillNameWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

void ReviewDialog::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    WindowManager *wm = environment.mWindowManager;

    // Add a line separator if there are items above
    if (!skillWidgets.empty())
    {
        addSeparator(coord1, coord2);
    }

    addGroup(wm->getGameSettingString(titleId, titleDefault), coord1, coord2);

    SkillList::const_iterator end = skills.end();
    for (SkillList::const_iterator it = skills.begin(); it != end; ++it)
    {
        int skillId = *it;
        if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
            continue;
        assert(skillId >= 0 && skillId < ESM::Skill::Length);
        const std::string &skillNameId = ESMS::Skill::sSkillNameIds[skillId];
        const MWMechanics::Stat<float> &stat = skillValues.find(skillId)->second;
        float base = stat.getBase();
        float modified = stat.getModified();

        ColorStyle style = CS_Normal;
        if (modified > base)
            style = CS_Super;
        else if (modified < base)
            style = CS_Sub;
        MyGUI::StaticTextPtr widget = addValueItem(wm->getGameSettingString(skillNameId, skillNameId), boost::lexical_cast<std::string>(static_cast<int>(modified)), style, coord1, coord2);
        skillWidgetMap[skillId] = widget;
    }
}

void ReviewDialog::updateSkillArea()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = skillWidgets.begin(); it != skillWidgets.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    skillWidgets.clear();

    const int valueSize = 40;
    MyGUI::IntCoord coord1(10, 0, skillClientWidget->getWidth() - (10 + valueSize), 18);
    MyGUI::IntCoord coord2(coord1.left + coord1.width, coord1.top, valueSize, coord1.height);

    if (!majorSkills.empty())
        addSkills(majorSkills, "sSkillClassMajor", "Major Skills", coord1, coord2);

    if (!minorSkills.empty())
        addSkills(minorSkills, "sSkillClassMinor", "Minor Skills", coord1, coord2);

    if (!miscSkills.empty())
        addSkills(miscSkills, "sSkillClassMisc", "Misc Skills", coord1, coord2);

    clientHeight = coord1.top;
    updateScroller();
}

void ReviewDialog::updateScroller()
{
    skillScrollerWidget->setScrollRange(std::max(clientHeight - skillClientWidget->getHeight(), 0));
    skillScrollerWidget->setScrollPage(std::max(skillClientWidget->getHeight() - lineHeight, 0));
}

// widget controls

void ReviewDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void ReviewDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void ReviewDialog::onNameClicked(MyGUI::Widget* _sender)
{
    eventNameActivated();
}

void ReviewDialog::onRaceClicked(MyGUI::Widget* _sender)
{
    eventRaceActivated();
}

void ReviewDialog::onClassClicked(MyGUI::Widget* _sender)
{
    eventClassActivated();
}

void ReviewDialog::onBirthSignClicked(MyGUI::Widget* _sender)
{
    eventBirthSignActivated();
}
