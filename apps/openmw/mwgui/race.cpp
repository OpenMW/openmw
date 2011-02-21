#include "race.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

RaceDialog::RaceDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_race_layout.xml", parWindowManager)
  , genderIndex(0)
  , faceIndex(0)
  , hairIndex(0)
  , faceCount(10)
  , hairCount(14)
{
    // Centre dialog
    center();

    // These are just demo values, you should replace these with
    // real calls from outside the class later.

    setText("AppearanceT", mWindowManager.getGameSettingString("sRaceMenu1", "Appearance"));
    getWidget(appearanceBox, "AppearanceBox");

    getWidget(headRotate, "HeadRotate");
    headRotate->setScrollRange(50);
    headRotate->setScrollPosition(20);
    headRotate->setScrollViewPage(10);
    headRotate->eventScrollChangePosition = MyGUI::newDelegate(this, &RaceDialog::onHeadRotate);

    // Set up next/previous buttons
    MyGUI::ButtonPtr prevButton, nextButton;

    setText("GenderChoiceT", mWindowManager.getGameSettingString("sRaceMenu2", "Change Sex"));
    getWidget(prevButton, "PrevGenderButton");
    getWidget(nextButton, "NextGenderButton");
    prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousGender);
    nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextGender);

    setText("FaceChoiceT", mWindowManager.getGameSettingString("sRaceMenu3", "Change Face"));
    getWidget(prevButton, "PrevFaceButton");
    getWidget(nextButton, "NextFaceButton");
    prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousFace);
    nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextFace);

    setText("HairChoiceT", mWindowManager.getGameSettingString("sRaceMenu3", "Change Hair"));
    getWidget(prevButton, "PrevHairButton");
    getWidget(nextButton, "NextHairButton");
    prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousHair);
    nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextHair);

    setText("RaceT", mWindowManager.getGameSettingString("sRaceMenu4", "Race"));
    getWidget(raceList, "RaceList");
    raceList->setScrollVisible(true);
    raceList->eventListSelectAccept = MyGUI::newDelegate(this, &RaceDialog::onSelectRace);
    raceList->eventListMouseItemActivate = MyGUI::newDelegate(this, &RaceDialog::onSelectRace);
    raceList->eventListChangePosition = MyGUI::newDelegate(this, &RaceDialog::onSelectRace);

    setText("SkillsT", mWindowManager.getGameSettingString("sBonusSkillTitle", "Skill Bonus"));
    getWidget(skillList, "SkillList");
    setText("SpellPowerT", mWindowManager.getGameSettingString("sRaceMenu7", "Specials"));
    getWidget(spellPowerList, "SpellPowerList");

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onOkClicked);

    updateRaces();
    updateSkills();
    updateSpellPowers();
}

void RaceDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    // TODO: All hardcoded coords for buttons are temporary, will be replaced with a dynamic system.
    if (shown)
    {
        okButton->setCaption("Next");

        // Adjust back button when next is shown
        backButton->setCoord(MyGUI::IntCoord(471 - 18, 397, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(532 - 18, 397, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        backButton->setCoord(MyGUI::IntCoord(471, 397, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(532, 397, 42, 23));
    }
}

void RaceDialog::open()
{
    updateRaces();
    updateSkills();
    updateSpellPowers();
    setVisible(true);
}


void RaceDialog::setRaceId(const std::string &raceId)
{
    currentRaceId = raceId;
    raceList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = raceList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*raceList->getItemDataAt<std::string>(i), raceId))
        {
            raceList->setIndexSelected(i);
            break;
        }
    }

    updateSkills();
    updateSpellPowers();
}

int wrap(int index, int max)
{
    if (index < 0)
        return max - 1;
    else if (index >= max)
        return 0;
    else
        return index;
}

// widget controls

void RaceDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void RaceDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void RaceDialog::onHeadRotate(MyGUI::VScroll*, size_t _position)
{
    // TODO: Rotate head
}

void RaceDialog::onSelectPreviousGender(MyGUI::Widget*)
{
    genderIndex = wrap(genderIndex - 1, 2);
}

void RaceDialog::onSelectNextGender(MyGUI::Widget*)
{
    genderIndex = wrap(genderIndex + 1, 2);
}

void RaceDialog::onSelectPreviousFace(MyGUI::Widget*)
{
    faceIndex = wrap(faceIndex - 1, faceCount);
}

void RaceDialog::onSelectNextFace(MyGUI::Widget*)
{
    faceIndex = wrap(faceIndex + 1, faceCount);
}

void RaceDialog::onSelectPreviousHair(MyGUI::Widget*)
{
    hairIndex = wrap(hairIndex - 1, hairCount);
}

void RaceDialog::onSelectNextHair(MyGUI::Widget*)
{
    hairIndex = wrap(hairIndex - 1, hairCount);
}

void RaceDialog::onSelectRace(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    const std::string *raceId = raceList->getItemDataAt<std::string>(_index);
    if (boost::iequals(currentRaceId, *raceId))
        return;

    currentRaceId = *raceId;
    updateSkills();
    updateSpellPowers();
}

// update widget content

void RaceDialog::updateRaces()
{
    raceList->removeAllItems();

    ESMS::ESMStore &store = mWindowManager.getStore();
    
    ESMS::RecListT<ESM::Race>::MapType::const_iterator it = store.races.list.begin();
    ESMS::RecListT<ESM::Race>::MapType::const_iterator end = store.races.list.end();
    int index = 0;
    for (; it != end; ++it)
    {
        const ESM::Race &race = it->second;
        bool playable = race.data.flags & ESM::Race::Playable;
        if (!playable) // Only display playable races
            continue;

        raceList->addItem(race.name, it->first);
        if (boost::iequals(it->first, currentRaceId))
            raceList->setIndexSelected(index);
        ++index;
    }
}

void RaceDialog::updateSkills()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = skillItems.begin(); it != skillItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    skillItems.clear();

    if (currentRaceId.empty())
        return;

    MWSkillPtr skillWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord1(0, 0, skillList->getWidth(), 18);

    ESMS::ESMStore &store = mWindowManager.getStore();
    const ESM::Race *race = store.races.find(currentRaceId);
    int count = sizeof(race->data.bonus)/sizeof(race->data.bonus[0]); // TODO: Find a portable macro for this ARRAYSIZE?
    for (int i = 0; i < count; ++i)
    {
        int skillId = race->data.bonus[i].skill;
        if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
            continue;

        skillWidget = skillList->createWidget<MWSkill>("MW_StatNameValue", coord1, MyGUI::Align::Default,
                                                       std::string("Skill") + boost::lexical_cast<std::string>(i));
        skillWidget->setWindowManager(&mWindowManager);
        skillWidget->setSkillNumber(skillId);
        skillWidget->setSkillValue(MWSkill::SkillValue(race->data.bonus[i].bonus));

        skillItems.push_back(skillWidget);

        coord1.top += lineHeight;
    }
}

void RaceDialog::updateSpellPowers()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = spellPowerItems.begin(); it != spellPowerItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    spellPowerItems.clear();

    if (currentRaceId.empty())
        return;

    MWSpellPtr spellPowerWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord(0, 0, spellPowerList->getWidth(), 18);

    ESMS::ESMStore &store = mWindowManager.getStore();
    const ESM::Race *race = store.races.find(currentRaceId);

    std::vector<std::string>::const_iterator it = race->powers.list.begin();
    std::vector<std::string>::const_iterator end = race->powers.list.end();
    for (int i = 0; it != end; ++it)
    {
        const std::string &spellpower = *it;
        spellPowerWidget = spellPowerList->createWidget<MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("SpellPower") + boost::lexical_cast<std::string>(i));
        spellPowerWidget->setWindowManager(&mWindowManager);
        spellPowerWidget->setSpellId(spellpower);

        spellPowerItems.push_back(spellPowerWidget);

        coord.top += lineHeight;
        ++i;
    }
}
