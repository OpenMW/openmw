#include "layouts.hpp"

#include "../mwworld/class.hpp"
#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwgui/window_manager.hpp"

#include <boost/lexical_cast.hpp>

using namespace MWGui;

const int StatsWindow::lineHeight = 18;

void StatsWindow::setStyledText(MyGUI::WidgetPtr widget, ColorStyle style, const std::string &value)
{
    widget->setCaption(value);
    if (style == CS_Super)
        widget->setTextColour(MyGUI::Colour(0, 1, 0));
    else if (style == CS_Sub)
        widget->setTextColour(MyGUI::Colour(1, 0, 0));
    else
        widget->setTextColour(MyGUI::Colour(1, 1, 1));
}

void StatsWindow::setValue (const std::string& id, const MWMechanics::Stat<float>& value)
{
    static struct {const char *id; ESM::Skill::SkillEnum skillId; } skillMap[] =
    {
        {"SkillBlock", ESM::Skill::Block},
        {"SkillArmorer", ESM::Skill::Armorer},
        {"SkillMediumArmor", ESM::Skill::MediumArmor},
        {"SkillHeavyArmor", ESM::Skill::HeavyArmor},
        {"SkillBluntWeapon", ESM::Skill::BluntWeapon},
        {"SkillLongBlade", ESM::Skill::LongBlade},
        {"SkillAxe", ESM::Skill::Axe},
        {"SkillSpear", ESM::Skill::Spear},
        {"SkillAthletics", ESM::Skill::Athletics},
        {"SkillEnchant", ESM::Skill::Armorer},
        {"SkillDestruction", ESM::Skill::Destruction},
        {"SkillAlteration", ESM::Skill::Alteration},
        {"SkillIllusion", ESM::Skill::Illusion},
        {"SkillConjuration", ESM::Skill::Conjuration},
        {"SkillMysticism", ESM::Skill::Mysticism},
        {"SkillRestoration", ESM::Skill::Restoration},
        {"SkillAlchemy", ESM::Skill::Alchemy},
        {"SkillUnarmored", ESM::Skill::Unarmored},
        {"SkillSecurity", ESM::Skill::Security},
        {"SkillSneak", ESM::Skill::Sneak},
        {"SkillAcrobatics", ESM::Skill::Acrobatics},
        {"SkillLightArmor", ESM::Skill::LightArmor},
        {"SkillShortBlade", ESM::Skill::ShortBlade},
        {"SkillMarksman", ESM::Skill::Marksman},
        {"SkillMercantile", ESM::Skill::Mercantile},
        {"SkillSpeechcraft", ESM::Skill::Speechcraft},
        {"SkillHandToHand", ESM::Skill::HandToHand},
    };
    for (int i = 0; i < sizeof(skillMap)/sizeof(skillMap[0]); ++i)
    {
        if (skillMap[i].id == id)
        {
            int skillId = skillMap[i].skillId;
            skillValues[skillId] = value;
            MyGUI::WidgetPtr widget = skillWidgetMap[skillId];
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
            break;
        }
    }
}

void StatsWindow::configureSkills (const std::set<int>& major, const std::set<int>& minor, const std::set<int>& misc)
{
    majorSkills = major;
    minorSkills = minor;
    miscSkills = misc;
}

void StatsWindow::configureFactions (const std::vector<std::string>& factions)
{
    this->factions = factions;
}

void StatsWindow::configureBirthSign (const std::string& signId)
{
    birthSignId = signId;
}

void StatsWindow::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticImagePtr separator = skillAreaWidget->createWidget<MyGUI::StaticImage>("MW_HLine", MyGUI::IntCoord(2 + 10, coord1.top, coord1.width + coord2.width - 8, 18), MyGUI::Align::Default);
    skillWidgets.push_back(separator);

    coord1.top += separator->getHeight();
    coord2.top += separator->getHeight();
}

void StatsWindow::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr groupWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandBrightText", MyGUI::IntCoord(4, coord1.top, coord1.width + coord2.width, coord1.height), MyGUI::Align::Default);
    groupWidget->setCaption(label);
    skillWidgets.push_back(groupWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

MyGUI::WidgetPtr StatsWindow::addValueItem(const std::string text, const std::string &value, ColorStyle style, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr skillNameWidget, skillValueWidget;

    skillNameWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandText", coord1, MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillValueWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandTextRight", coord2, MyGUI::Align::Default);
    setStyledText(skillValueWidget, style, value);

    skillWidgets.push_back(skillNameWidget);
    skillWidgets.push_back(skillValueWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;

    return skillValueWidget;
}

void StatsWindow::addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    MyGUI::StaticTextPtr skillNameWidget;

    skillNameWidget = skillAreaWidget->createWidget<MyGUI::StaticText>("SandText", coord1 + MyGUI::IntSize(coord2.width, 0), MyGUI::Align::Default);
    skillNameWidget->setCaption(text);

    skillWidgets.push_back(skillNameWidget);

    coord1.top += lineHeight;
    coord2.top += lineHeight;
}

void StatsWindow::addSkills(const std::set<int> &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
{
    WindowManager *wm = environment.mWindowManager;
    MWMechanics::MechanicsManager *mm = environment.mMechanicsManager;
    ESMS::ESMStore &store = environment.mWorld->getStore();

    // Add a line separator if there are items above
    if (!skillWidgets.empty())
    {
        addSeparator(coord1, coord2);
    }

    addGroup(wm->getGameSettingString(titleId, titleDefault), coord1, coord2);

    std::set<int>::const_iterator end = skills.end();
    for (std::set<int>::const_iterator it = skills.begin(); it != end; ++it)
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
        MyGUI::WidgetPtr widget = addValueItem(wm->getGameSettingString(skillNameId, skillNameId), boost::lexical_cast<std::string>(static_cast<int>(modified)), style, coord1, coord2);
        skillWidgetMap[skillId] = widget;
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

    WindowManager *wm = environment.mWindowManager;
    ESMS::ESMStore &store = environment.mWorld->getStore();

    if (!factions.empty())
    {
        // Add a line separator if there are items above
        if (!skillWidgets.empty())
            addSeparator(coord1, coord2);

        addGroup(wm->getGameSettingString("sFaction", "Faction"), coord1, coord2);
        std::vector<std::string>::const_iterator end = factions.end();
        for (std::vector<std::string>::const_iterator it = factions.begin(); it != end; ++it)
        {
            const ESM::Faction *faction = store.factions.find(*it);
            addItem(faction->name, coord1, coord2);
        }
    }

    if (!birthSignId.empty())
    {
        // Add a line separator if there are items above
        if (!skillWidgets.empty())
            addSeparator(coord1, coord2);

        addGroup(wm->getGameSettingString("sSign", "Sign"), coord1, coord2);
        const ESM::BirthSign *sign = store.birthSigns.find(birthSignId);
        addItem(sign->name, coord1, coord2);
    }

    // Add a line separator if there are items above
    if (!skillWidgets.empty())
        addSeparator(coord1, coord2);

    addValueItem(wm->getGameSettingString("sReputation", "Reputation"), boost::lexical_cast<std::string>(static_cast<int>(reputation)), CS_Normal, coord1, coord2);
    addValueItem(wm->getGameSettingString("sBounty", "Bounty"), boost::lexical_cast<std::string>(static_cast<int>(bounty)), CS_Normal, coord1, coord2);
}
