#include "layouts.hpp"

#include "../mwworld/class.hpp"
#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwgui/window_manager.hpp"

#include <boost/lexical_cast.hpp>

using namespace MWGui;

void StatsWindow::configureSkills (const std::set<int>& major, const std::set<int>& minor, const std::set<int>& misc)
{
    majorSkills = major;
    minorSkills = minor;
    miscSkills = misc;

    updateSkillArea();
}

void StatsWindow::addSkills(const std::set<int> &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    // Get player and stats
    MWWorld::Ptr ptr = environment.mWorld->getPlayerPos().getPlayer();
    MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
    MWMechanics::NpcStats& npcStats = MWWorld::Class::get (ptr).getNpcStats (ptr);

    WindowManager *wm = environment.mWindowManager;
    MWMechanics::MechanicsManager *mm = environment.mMechanicsManager;
    ESMS::ESMStore &store = environment.mWorld->getStore();

    MyGUI::StaticTextPtr skillNameWidget, skillValueWidget;
    const int lineHeight = 18;

    // Add a line separator if there are items above
    if (!skillWidgets.empty())
    {
        MyGUI::StaticImagePtr separator = skillAreaWidget->createWidget<MyGUI::StaticImage>("MW_HLine", MyGUI::IntCoord(2 + 10, coord1.top, coord1.width + coord2.width - 8, 18), MyGUI::Align::Default);
        skillWidgets.push_back(separator);
        coord1.top += separator->getHeight();
        coord2.top += separator->getHeight();
    }

    skillNameWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandBrightText", MyGUI::IntCoord(4, coord1.top, coord1.width + coord2.width, coord1.height), MyGUI::Align::Default);
    skillNameWidget->setCaption(wm->getGameSettingString(titleId, titleDefault));
    skillWidgets.push_back(skillNameWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    int i = 0;
    std::set<int>::const_iterator end = skills.end();
    for (std::set<int>::const_iterator it = skills.begin(); it != end; ++it)
    {
        int skillId = *it;
        if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
            continue;
        assert(skillId >= 0 && skillId < ESM::Skill::Length);
        const std::string &skillNameId = ESMS::Skill::sSkillNameIds[skillId];
        assert(skillId < sizeof(npcStats.mSkill)/sizeof(npcStats.mSkill[0]));
        MWMechanics::Stat<float> &stat = npcStats.mSkill[skillId];
        float base = stat.getBase();
        float modified = stat.getModified();

        skillNameWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandText", coord1, MyGUI::Align::Default,
                                                                           std::string("SkillName") + boost::lexical_cast<std::string>(i));
        skillNameWidget->setCaption(wm->getGameSettingString(skillNameId, skillNameId));

        skillValueWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandTextRight", coord2, MyGUI::Align::Default,
                                                                            std::string("SkillValue") + boost::lexical_cast<std::string>(i));
        skillValueWidget->setCaption(boost::lexical_cast<std::string>(static_cast<int>(modified)));
        if (modified > base)
            skillValueWidget->setTextColour(MyGUI::Colour(0, 1, 0));
        else if (modified < base)
            skillValueWidget->setTextColour(MyGUI::Colour(1, 0, 0));
        else
            skillValueWidget->setTextColour(MyGUI::Colour(1, 1, 1));

        skillWidgets.push_back(skillNameWidget);
        skillWidgets.push_back(skillValueWidget);

        coord1.top += lineHeight;
        coord2.top += lineHeight;
        ++i;
    }
}

void StatsWindow::updateSkillArea()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = skillWidgets.begin(); it != skillWidgets.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    skillWidgets.clear();

    const int valueSize = 40;
    MyGUI::IntCoord coord1(14, 4, skillAreaWidget->getWidth() - (14 + valueSize + 4), 18);
    MyGUI::IntCoord coord2(coord1.left + coord1.width, coord1.top, valueSize, coord1.height);

    if (!majorSkills.empty())
        addSkills(majorSkills, "sSkillClassMajor", "Major Skills", coord1, coord2);

    if (!minorSkills.empty())
        addSkills(minorSkills, "sSkillClassMinor", "Minor Skills", coord1, coord2);

    if (!miscSkills.empty())
        addSkills(miscSkills, "sSkillClassMisc", "Misc Skills", coord1, coord2);
}
