#include "mw_chargen.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;

RaceDialog::RaceDialog(MWWorld::Environment& environment)
  : Layout("openmw_chargen_race_layout.xml")
  , environment(environment)
  , genderIndex(0)
  , faceIndex(0)
  , hairIndex(0)
  , faceCount(10)
  , hairCount(14)
{
	mMainWidget->setCoord(mMainWidget->getCoord() + MyGUI::IntPoint(0, 100));

	// These are just demo values, you should replace these with
	// real calls from outside the class later.

	setText("AppearanceT", "Appearance");
	getWidget(appearanceBox, "AppearanceBox");

	getWidget(headRotate, "HeadRotate");
	headRotate->setScrollRange(50);
	headRotate->setScrollPosition(20);
	headRotate->setScrollViewPage(10);
	headRotate->eventScrollChangePosition = MyGUI::newDelegate(this, &RaceDialog::onHeadRotate);

	// Set up next/previous buttons
	MyGUI::ButtonPtr prevButton, nextButton;

	getWidget(prevButton, "PrevGenderButton");
	getWidget(nextButton, "NextGenderButton");
	prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousGender);
	nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextGender);

	getWidget(prevButton, "PrevFaceButton");
	getWidget(nextButton, "NextFaceButton");
	prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousFace);
	nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextFace);

	getWidget(prevButton, "PrevHairButton");
	getWidget(nextButton, "NextHairButton");
	prevButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousHair);
	nextButton->eventMouseButtonClick = MyGUI::newDelegate(this, &RaceDialog::onSelectNextHair);

	setText("RaceT", "Race");
	getWidget(raceList, "RaceList");
	raceList->setScrollVisible(true);
	raceList->eventListSelectAccept = MyGUI::newDelegate(this, &RaceDialog::onSelectRace);
	raceList->eventListMouseItemActivate = MyGUI::newDelegate(this, &RaceDialog::onSelectRace);

	getWidget(skillList, "SkillList");
	getWidget(spellPowerList, "SpellPowerList");

	updateRaces();
	updateSkills();
	updateSpellPowers();
}

void RaceDialog::setRace(const std::string &race)
{
    currentRace = race;
    raceList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = raceList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(raceList->getItem(i), race))
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
    const std::string race = raceList->getItem(_index);
    if (boost::iequals(currentRace, race))
        return;

    currentRace = race;
    updateSkills();
	updateSpellPowers();
}

// update widget content

void RaceDialog::updateRaces()
{
	raceList->removeAllItems();

    ESMS::ESMStore &store = environment.mWorld->getStore();
	
    ESMS::RecListT<ESM::Race>::MapType::const_iterator it = store.races.list.begin();
    ESMS::RecListT<ESM::Race>::MapType::const_iterator end = store.races.list.end();
	int index = 0;
	for (; it != end; ++it)
	{
        const ESM::Race &race = it->second;
        bool playable = race.data.flags & ESM::Race::Playable;
        if (!playable) // Only display playable races
            continue;

        raceList->addItem(race.name);
		if (boost::iequals(race.name, currentRace))
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

    if (currentRace.empty())
        return;

	MyGUI::StaticTextPtr skillNameWidget, skillBonusWidget;
	const int lineHeight = 18;
	MyGUI::IntCoord coord1(0, 0, skillList->getWidth() - (40 + 4), 18);
	MyGUI::IntCoord coord2(coord1.left + coord1.width, 0, 40, 18);

    ESMS::ESMStore &store = environment.mWorld->getStore();
    const ESM::Race *race = store.races.find(currentRace);
    int count = sizeof(race->data.bonus)/sizeof(race->data.bonus[0]); // TODO: Find a portable macro for this ARRAYSIZE?
    for (int i = 0; i < count; ++i)
	{
        int skillId = race->data.bonus[i].skill;
        if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
            continue;

        skillNameWidget = skillList->createWidget<MyGUI::StaticText>("SandText", coord1, MyGUI::Align::Default,
                                                                     std::string("SkillName") + boost::lexical_cast<std::string>(i));
        assert(skillId >= 0 && skillId < ESM::Skill::Length);
		skillNameWidget->setCaption(ESMS::Skill::sSkillNames[skillId]);

		skillBonusWidget = skillList->createWidget<MyGUI::StaticText>("SandTextRight", coord2, MyGUI::Align::Default,
                                                                      std::string("SkillBonus") + boost::lexical_cast<std::string>(i));
        int bonus = race->data.bonus[i].bonus;
		skillBonusWidget->setCaption(boost::lexical_cast<std::string>(bonus));

		skillItems.push_back(skillNameWidget);
		skillItems.push_back(skillBonusWidget);

		coord1.top += lineHeight;
		coord2.top += lineHeight;
	}
}

void RaceDialog::updateSpellPowers()
{
	for (std::vector<MyGUI::WidgetPtr>::iterator it = spellPowerItems.begin(); it != spellPowerItems.end(); ++it)
	{
		MyGUI::Gui::getInstance().destroyWidget(*it);
	}
	spellPowerItems.clear();

    if (currentRace.empty())
        return;

	MyGUI::StaticTextPtr spellPowerWidget;
	const int lineHeight = 18;
	MyGUI::IntCoord coord(0, 0, spellPowerList->getWidth(), 18);

    ESMS::ESMStore &store = environment.mWorld->getStore();
    const ESM::Race *race = store.races.find(currentRace);

    std::vector<std::string>::const_iterator it = race->powers.list.begin();
    std::vector<std::string>::const_iterator end = race->powers.list.end();
	for (int i = 0; it != end; ++it)
	{
        const std::string &spellpower = *it;
        const ESM::Spell *spell = store.spells.find(spellpower);
        assert(spell);
        spellPowerWidget = spellPowerList->createWidget<MyGUI::StaticText>("SandText", coord, MyGUI::Align::Default, std::string("SpellPowerName") + boost::lexical_cast<std::string>(i));
		spellPowerWidget->setCaption(spell->name);

		spellPowerItems.push_back(spellPowerWidget);

		coord.top += lineHeight;
        ++i;
	}
}
