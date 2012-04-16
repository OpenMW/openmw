#include "review.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <cmath>

#undef min
#undef max

using namespace MWGui;
using namespace Widgets;

const int ReviewDialog::lineHeight = 18;

ReviewDialog::ReviewDialog(WindowManager& parWindowManager)
    : WindowBase("openmw_chargen_review_layout.xml", parWindowManager)
    , lastPos(0)
{
    // Centre dialog
    center();

    // Setup static stats
    ButtonPtr button;
    getWidget(nameWidget, "NameText");
    getWidget(button, "NameButton");
    button->setCaption(mWindowManager.getGameSettingString("sName", ""));
    adjustButtonSize(button);
    button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onNameClicked);;

    getWidget(raceWidget, "RaceText");
    getWidget(button, "RaceButton");
    button->setCaption(mWindowManager.getGameSettingString("sRace", ""));
    adjustButtonSize(button);
    button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onRaceClicked);;

    getWidget(classWidget, "ClassText");
    getWidget(button, "ClassButton");
    button->setCaption(mWindowManager.getGameSettingString("sClass", ""));
    adjustButtonSize(button);
    button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onClassClicked);;

    getWidget(birthSignWidget, "SignText");
    getWidget(button, "SignButton");
    button->setCaption(mWindowManager.getGameSettingString("sBirthSign", ""));
    adjustButtonSize(button);
    button->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onBirthSignClicked);;

    // Setup dynamic stats
    getWidget(health, "Health");
    health->setTitle(mWindowManager.getGameSettingString("sHealth", ""));
    health->setValue(45, 45);

    getWidget(magicka, "Magicka");
    magicka->setTitle(mWindowManager.getGameSettingString("sMagic", ""));
    magicka->setValue(50, 50);

    getWidget(fatigue, "Fatigue");
    fatigue->setTitle(mWindowManager.getGameSettingString("sFatigue", ""));
    fatigue->setValue(160, 160);

    // Setup attributes

    MWAttributePtr attribute;
    for (int idx = 0; idx < ESM::Attribute::Length; ++idx)
    {
        getWidget(attribute, std::string("Attribute") + boost::lexical_cast<std::string>(idx));
        attributeWidgets.insert(std::make_pair(static_cast<int>(ESM::Attribute::attributeIds[idx]), attribute));
        attribute->setWindowManager(&mWindowManager);
        attribute->setAttributeId(ESM::Attribute::attributeIds[idx]);
        attribute->setAttributeValue(MWAttribute::AttributeValue(0, 0));
    }

    // Setup skills
    getWidget(skillAreaWidget, "Skills");
    getWidget(skillClientWidget, "SkillClient");
    getWidget(skillScrollerWidget, "SkillScroller");

    skillScrollerWidget->eventScrollChangePosition += MyGUI::newDelegate(this, &ReviewDialog::onScrollChangePosition);
    updateScroller();

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        skillValues.insert(std::make_pair(i, MWMechanics::Stat<float>()));
        skillWidgetMap.insert(std::make_pair(i, static_cast<MyGUI::TextBox*> (0)));
    }

    static_cast<MyGUI::WindowPtr>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &ReviewDialog::onWindowResize);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ReviewDialog::onOkClicked);
}

void ReviewDialog::open()
{
    updateSkillArea();
    setVisible(true);
}

void ReviewDialog::onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
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

void ReviewDialog::onWindowResize(MyGUI::Window* window)
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
    const ESM::Race *race = mWindowManager.getStore().races.search(raceId);
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
    const ESM::BirthSign *sign = mWindowManager.getStore().birthSigns.search(birthSignId);
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
    MyGUI::TextBox* widget = skillWidgetMap[skillId];
    if (widget)
    {
        float modified = value.getModified(), base = value.getBase();
        std::string text = boost::lexical_cast<std::string>(std::floor(modified));
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

void ReviewDialog::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::ImageBox* separator = skillClientWidget->createWidget<MyGUI::ImageBox>("MW_HLine", MyGUI::IntCoord(10, coord1.top, coord1.width + coord2.width - 4, 18), MyGUI::Align::Default);
    skillWidgets.push_back(separator);

    coord1.top += separator->getHeight();
    coord2.top += separator->getHeight();
}

void ReviewDialog::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox* groupWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, coord1.top, coord1.width + coord2.width, coord1.height), MyGUI::Align::Default);
    groupWidget->setCaption(label);
    skillWidgets.push_back(groupWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

MyGUI::TextBox* ReviewDialog::addValueItem(const std::string text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox* skillNameWidget;
    MyGUI::TextBox* skillValueWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillValueWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandTextRight", coord2, MyGUI::Align::Default);
    skillValueWidget->setCaption(value);
    skillValueWidget->_setWidgetState(state);

    skillWidgets.push_back(skillNameWidget);
    skillWidgets.push_back(skillValueWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    return skillValueWidget;
}

void ReviewDialog::addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox* skillNameWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillWidgets.push_back(skillNameWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

void ReviewDialog::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    // Add a line separator if there are items above
    if (!skillWidgets.empty())
    {
        addSeparator(coord1, coord2);
    }

    addGroup(mWindowManager.getGameSettingString(titleId, titleDefault), coord1, coord2);

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

        std::string state = "normal";
        if (modified > base)
            state = "increased";
        else if (modified < base)
            state = "decreased";
        MyGUI::TextBox* widget = addValueItem(mWindowManager.getGameSettingString(skillNameId, skillNameId), boost::lexical_cast<std::string>(static_cast<int>(modified)), state, coord1, coord2);
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
