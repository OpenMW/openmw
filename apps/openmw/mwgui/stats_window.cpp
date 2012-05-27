#include "stats_window.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>

#include <boost/lexical_cast.hpp>

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwbase/environment.hpp"

#include "window_manager.hpp"
#include "tooltips.hpp"


using namespace MWGui;
const int StatsWindow::lineHeight = 18;

StatsWindow::StatsWindow (WindowManager& parWindowManager)
  : WindowPinnableBase("openmw_stats_window_layout.xml", parWindowManager)
  , skillAreaWidget(NULL)
  , skillClientWidget(NULL)
  , skillScrollerWidget(NULL)
  , lastPos(0)
  , clientHeight(0)
  , majorSkills()
  , minorSkills()
  , miscSkills()
  , skillValues()
  , skillWidgetMap()
  , factionWidgetMap()
  , mFactions()
  , birthSignId()
  , reputation(0)
  , bounty(0)
  , skillWidgets()
  , mChanged(true)
{
    setCoord(0,0,498, 342);

    const char *names[][2] =
    {
        { "Attrib1", "sAttributeStrength" },
        { "Attrib2", "sAttributeIntelligence" },
        { "Attrib3", "sAttributeWillpower" },
        { "Attrib4", "sAttributeAgility" },
        { "Attrib5", "sAttributeSpeed" },
        { "Attrib6", "sAttributeEndurance" },
        { "Attrib7", "sAttributePersonality" },
        { "Attrib8", "sAttributeLuck" },
        { 0, 0 }
    };

    const ESMS::ESMStore &store = mWindowManager.getStore();
    for (int i=0; names[i][0]; ++i)
    {
        setText (names[i][0], store.gameSettings.find (names[i][1])->str);
    }

    getWidget(skillAreaWidget, "Skills");
    getWidget(skillClientWidget, "SkillClient");
    getWidget(skillScrollerWidget, "SkillScroller");
    getWidget(mLeftPane, "LeftPane");
    getWidget(mRightPane, "RightPane");

    skillClientWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

    skillScrollerWidget->eventScrollChangePosition += MyGUI::newDelegate(this, &StatsWindow::onScrollChangePosition);
    updateScroller();

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        skillValues.insert(std::pair<int, MWMechanics::Stat<float> >(i, MWMechanics::Stat<float>()));
        skillWidgetMap.insert(std::pair<int, MyGUI::TextBox*>(i, nullptr));
    }

    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowChangeCoord += MyGUI::newDelegate(this, &StatsWindow::onWindowResize);
}

void StatsWindow::onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
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

void StatsWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (skillScrollerWidget->getScrollPosition() - _rel*0.3 < 0)
        skillScrollerWidget->setScrollPosition(0);
    else if (skillScrollerWidget->getScrollPosition() - _rel*0.3 > skillScrollerWidget->getScrollRange()-1)
        skillScrollerWidget->setScrollPosition(skillScrollerWidget->getScrollRange()-1);
    else
        skillScrollerWidget->setScrollPosition(skillScrollerWidget->getScrollPosition() - _rel*0.3);

    onScrollChangePosition(skillScrollerWidget, skillScrollerWidget->getScrollPosition());
}

void StatsWindow::onWindowResize(MyGUI::Window* window)
{
    mLeftPane->setCoord( MyGUI::IntCoord(0, 0, 0.44*window->getSize().width, window->getSize().height) );
    mRightPane->setCoord( MyGUI::IntCoord(0.44*window->getSize().width, 0, 0.56*window->getSize().width, window->getSize().height) );
    updateScroller();
}

void StatsWindow::setBar(const std::string& name, const std::string& tname, int val, int max)
{
    MyGUI::ProgressPtr pt;
    getWidget(pt, name);
    pt->setProgressRange(max);
    pt->setProgressPosition(val);

    std::stringstream out;
    out << val << "/" << max;
    setText(tname, out.str().c_str());
}

void StatsWindow::setPlayerName(const std::string& playerName)
{
    static_cast<MyGUI::Window*>(mMainWidget)->setCaption(playerName);
    adjustWindowCaption();
}

void StatsWindow::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    static const char *ids[] =
    {
        "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
        "AttribVal6", "AttribVal7", "AttribVal8",
        0
    };

    for (int i=0; ids[i]; ++i)
        if (ids[i]==id)
        {
            std::ostringstream valueString;
            valueString << value.getModified();
            setText (id, valueString.str());

            MyGUI::TextBox* box;
            getWidget(box, id);

            if (value.getModified()>value.getBase())
                box->_setWidgetState("increased");
            else if (value.getModified()<value.getBase())
                box->_setWidgetState("decreased");
            else
                box->_setWidgetState("normal");

            break;
        }
}

void StatsWindow::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    static const char *ids[] =
    {
        "HBar", "MBar", "FBar",
        0
    };

    for (int i=0; ids[i]; ++i)
    {
        if (ids[i]==id)
        {
            std::string id (ids[i]);
            setBar (id, id + "T", value.getCurrent(), value.getModified());

            // health, magicka, fatigue tooltip
            MyGUI::Widget* w;
            std::string valStr =  boost::lexical_cast<std::string>(value.getCurrent()) + "/" + boost::lexical_cast<std::string>(value.getModified());
            if (i==0)
            {
                getWidget(w, "Health");
                w->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
            }
            else if (i==1)
            {
                getWidget(w, "Magicka");
                w->setUserString("Caption_HealthDescription", "#{sIntDesc}\n" + valStr);
            }
            else if (i==2)
            {
                getWidget(w, "Fatigue");
                w->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
            }
        }
    }
}

void StatsWindow::setValue (const std::string& id, const std::string& value)
{
    if (id=="name")
        setPlayerName (value);
    else if (id=="race")
        setText ("RaceText", value);
    else if (id=="class")
        setText ("ClassText", value);
}

void StatsWindow::setValue (const std::string& id, int value)
{
    if (id=="level")
    {
        std::ostringstream text;
        text << value;
        setText("LevelText", text.str());
    }
}

void StatsWindow::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value)
{
    skillValues[parSkill] = value;
    MyGUI::TextBox* widget = skillWidgetMap[(int)parSkill];
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

void StatsWindow::configureSkills (const std::vector<int>& major, const std::vector<int>& minor)
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

void StatsWindow::onFrame ()
{
    if (mMainWidget->getVisible())
        return;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
    MWMechanics::NpcStats PCstats = MWWorld::Class::get(player).getNpcStats(player);

    setFactions(PCstats.mFactionRank);

    setBirthSign(MWBase::Environment::get().getWorld()->getPlayer().getBirthsign());

    if (mChanged)
        updateSkillArea();
}

void StatsWindow::setFactions (const FactionList& factions)
{
    if (mFactions != factions)
    {
        mFactions = factions;
        mChanged = true;
    }
}

void StatsWindow::setBirthSign (const std::string& signId)
{
    if (signId != birthSignId)
    {
        birthSignId = signId;
        mChanged = true;
    }
}

void StatsWindow::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::ImageBox* separator = skillClientWidget->createWidget<MyGUI::ImageBox>("MW_HLine",
        MyGUI::IntCoord(10, coord1.top, coord1.width + coord2.width - 4, 18),
        MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
    separator->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);
    skillWidgets.push_back(separator);

    coord1.top += separator->getHeight();
    coord2.top += separator->getHeight();
}

void StatsWindow::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox* groupWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandBrightText",
        MyGUI::IntCoord(0, coord1.top, coord1.width + coord2.width, coord1.height),
        MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
    groupWidget->setCaption(label);
    groupWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);
    skillWidgets.push_back(groupWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

MyGUI::TextBox* StatsWindow::addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox *skillNameWidget, *skillValueWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
    skillNameWidget->setCaption(text);
    skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

    skillValueWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandTextRight", coord2, MyGUI::Align::Right | MyGUI::Align::Top);
    skillValueWidget->setCaption(value);
    skillValueWidget->_setWidgetState(state);
    skillValueWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

    skillWidgets.push_back(skillNameWidget);
    skillWidgets.push_back(skillValueWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    return skillValueWidget;
}

MyGUI::Widget* StatsWindow::addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::TextBox* skillNameWidget;

    skillNameWidget = skillClientWidget->createWidget<MyGUI::TextBox>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
    skillNameWidget->setCaption(text);
    skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

    skillWidgets.push_back(skillNameWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    return skillNameWidget;
}

void StatsWindow::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
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
        int progressPercent = (modified - float(static_cast<int>(modified))) * 100;

        const ESM::Skill* skill = mWindowManager.getStore().skills.search(skillId);
        assert(skill);

        std::string icon = "icons\\k\\" + ESM::Skill::sIconNames[skillId];

        const ESM::Attribute* attr = mWindowManager.getStore().attributes.search(skill->data.attribute);
        assert(attr);

        std::string state = "normal";
        if (modified > base)
            state = "increased";
        else if (modified < base)
            state = "decreased";
        MyGUI::TextBox* widget = addValueItem(mWindowManager.getGameSettingString(skillNameId, skillNameId),
            boost::lexical_cast<std::string>(static_cast<int>(modified)), state, coord1, coord2);

        for (int i=0; i<2; ++i)
        {
            skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipLayout", "SkillToolTip");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_SkillName", "#{"+skillNameId+"}");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_SkillDescription", skill->description);
            skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_SkillAttribute", "#{sGoverningAttribute}: #{" + attr->name + "}");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("ImageTexture_SkillImage", icon);
            skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_SkillProgressText", boost::lexical_cast<std::string>(progressPercent)+"/100");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("Range_SkillProgress", "100");
            skillWidgets[skillWidgets.size()-1-i]->setUserString("RangePosition_SkillProgress", boost::lexical_cast<std::string>(progressPercent));
        }

        skillWidgetMap[skillId] = widget;
    }
}

void StatsWindow::updateSkillArea()
{
    mChanged = false;

    for (std::vector<MyGUI::WidgetPtr>::iterator it = skillWidgets.begin(); it != skillWidgets.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    skillWidgets.clear();

    skillScrollerWidget->setScrollPosition(0);
    onScrollChangePosition(skillScrollerWidget, 0);
    clientHeight = 0;

    const int valueSize = 40;
    MyGUI::IntCoord coord1(10, 0, skillClientWidget->getWidth() - (10 + valueSize), 18);
    MyGUI::IntCoord coord2(coord1.left + coord1.width, coord1.top, valueSize, coord1.height);

    if (!majorSkills.empty())
        addSkills(majorSkills, "sSkillClassMajor", "Major Skills", coord1, coord2);

    if (!minorSkills.empty())
        addSkills(minorSkills, "sSkillClassMinor", "Minor Skills", coord1, coord2);

    if (!miscSkills.empty())
        addSkills(miscSkills, "sSkillClassMisc", "Misc Skills", coord1, coord2);

    const ESMS::ESMStore &store = mWindowManager.getStore();

    // race tooltip
    const ESM::Race* playerRace =  store.races.find (MWBase::Environment::get().getWorld()->getPlayer().getRace());
    MyGUI::Widget* raceWidget;
    getWidget(raceWidget, "RaceText");
    ToolTips::createRaceToolTip(raceWidget, playerRace);
    getWidget(raceWidget, "Race_str");
    ToolTips::createRaceToolTip(raceWidget, playerRace);

    // class tooltip
    MyGUI::Widget* classWidget;
    const ESM::Class& playerClass = MWBase::Environment::get().getWorld()->getPlayer().getClass();
    getWidget(classWidget, "ClassText");
    ToolTips::createClassToolTip(classWidget, playerClass);
    getWidget(classWidget, "Class_str");
    ToolTips::createClassToolTip(classWidget, playerClass);

    if (!mFactions.empty())
    {
        // Add a line separator if there are items above
        if (!skillWidgets.empty())
            addSeparator(coord1, coord2);

        addGroup(mWindowManager.getGameSettingString("sFaction", "Faction"), coord1, coord2);
        FactionList::const_iterator end = mFactions.end();
        for (FactionList::const_iterator it = mFactions.begin(); it != end; ++it)
        {
            const ESM::Faction *faction = store.factions.find(it->first);
            MyGUI::Widget* w = addItem(faction->name, coord1, coord2);

            std::string text;

            text += std::string("#DDC79E") + faction->name;
            text += std::string("\n#BF9959") + faction->ranks[it->second];

            if (it->second < 9)
            {
                // player doesn't have max rank yet
                text += std::string("\n\n#DDC79E#{sNextRank} ") + faction->ranks[it->second+1];

                ESM::RankData rankData = faction->data.rankData[it->second+1];
                const ESM::Attribute* attr1 = mWindowManager.getStore().attributes.search(faction->data.attribute1);
                const ESM::Attribute* attr2 = mWindowManager.getStore().attributes.search(faction->data.attribute2);
                assert(attr1 && attr2);

                text += "\n#BF9959#{" + attr1->name + "}: " + boost::lexical_cast<std::string>(rankData.attribute1)
                        + ", #{" + attr2->name + "}: " + boost::lexical_cast<std::string>(rankData.attribute2);

                text += "\n\n#DDC79E#{sFavoriteSkills}";
                text += "\n#BF9959";
                for (int i=0; i<6; ++i)
                {
                    const ESM::Skill* skill = mWindowManager.getStore().skills.search(faction->data.skillID[i]);
                    assert(skill);
                    text += "#{"+ESM::Skill::sSkillNameIds[faction->data.skillID[i]]+"}";
                    if (i<5)
                        text += ", ";
                }

                text += "\n";

                if (rankData.skill1 > 0)
                    text += "\n#{sNeedOneSkill} " + boost::lexical_cast<std::string>(rankData.skill1);
                if (rankData.skill2 > 0)
                    text += "\n#{sNeedTwoSkills} " + boost::lexical_cast<std::string>(rankData.skill2);
            }

            w->setUserString("ToolTipType", "Layout");
            w->setUserString("ToolTipLayout", "TextToolTip");
            w->setUserString("Caption_Text", text);
        }
    }

    if (!birthSignId.empty())
    {
        // Add a line separator if there are items above
        if (!skillWidgets.empty())
            addSeparator(coord1, coord2);

        addGroup(mWindowManager.getGameSettingString("sBirthSign", "Sign"), coord1, coord2);
        const ESM::BirthSign *sign = store.birthSigns.find(birthSignId);
        MyGUI::Widget* w = addItem(sign->name, coord1, coord2);

        ToolTips::createBirthsignToolTip(w, birthSignId);
    }

    // Add a line separator if there are items above
    if (!skillWidgets.empty())
        addSeparator(coord1, coord2);

    addValueItem(mWindowManager.getGameSettingString("sReputation", "Reputation"),
                boost::lexical_cast<std::string>(static_cast<int>(reputation)), "normal", coord1, coord2);

    for (int i=0; i<2; ++i)
    {
        skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
        skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipLayout", "TextToolTip");
        skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_Text", "#{sSkillsMenuReputationHelp}");
    }
    
    addValueItem(mWindowManager.getGameSettingString("sBounty", "Bounty"),
                boost::lexical_cast<std::string>(static_cast<int>(bounty)), "normal", coord1, coord2);

    for (int i=0; i<2; ++i)
    {
        skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
        skillWidgets[skillWidgets.size()-1-i]->setUserString("ToolTipLayout", "TextToolTip");
        skillWidgets[skillWidgets.size()-1-i]->setUserString("Caption_Text", "#{sCrimeHelp}");
    }

    clientHeight = coord1.top;
    updateScroller();
}

void StatsWindow::updateScroller()
{
    skillScrollerWidget->setScrollRange(std::max(clientHeight - skillClientWidget->getHeight(), 0));
    skillScrollerWidget->setScrollPage(std::max(skillClientWidget->getHeight() - lineHeight, 0));
    if (clientHeight != 0)
        skillScrollerWidget->setTrackSize( (skillAreaWidget->getHeight() / float(clientHeight)) * skillScrollerWidget->getLineSize() );
}

void StatsWindow::onPinToggled()
{
    mWindowManager.setHMSVisibility(!mPinned);
}
