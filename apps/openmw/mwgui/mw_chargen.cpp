#include "mw_chargen.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

RaceDialog::RaceDialog()
  : Layout("openmw_chargen_race_layout.xml")
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
	// TODO: Select actual race
	updateSkills();
	updateSpellPowers();
}

// update widget content

void RaceDialog::updateRaces()
{
	raceList->removeAllItems();
	raceList->addItem("Argonian");
	raceList->addItem("Breton");
	raceList->addItem("Dark Elf");
	raceList->addItem("High Elf");
	raceList->addItem("Imperial");
	raceList->addItem("Khajiit");
	raceList->addItem("Nord");
	raceList->addItem("Orc");
}

void RaceDialog::updateSkills()
{
	for (std::vector<MyGUI::WidgetPtr>::iterator it = skillItems.begin(); it != skillItems.end(); ++it)
	{
		MyGUI::Gui::getInstance().destroyWidget(*it);
	}
	skillItems.clear();

	MyGUI::StaticTextPtr skillName, skillBonus;
	const int lineHeight = 18;
	MyGUI::IntCoord coord1(0, 0, skillList->getWidth() - (40 + 4), 18);
	MyGUI::IntCoord coord2(coord1.left + coord1.width, 0, 40, 18);

	const char *inputList[][2] = {
		{"Athletics", "5"},
		{"Destruction", "10"},
		{"Light Armor", "5"},
		{"Long Blade", "5"},
		{"Marksman", "5"},
		{"Mysticism", "5"},
		{"Short Blade", "10"},
		{0,0}
	};

	for (int i = 0; inputList[i][0]; ++i)
	{
		std::ostringstream name;
		name << std::string("SkillName") << i;
		skillName = skillList->createWidget<MyGUI::StaticText>("SandText", coord1, MyGUI::Align::Default, name.str());
		skillName->setCaption(inputList[i][0]);
		std::ostringstream bonus;
		bonus << std::string("SkillBonus") << i;
		skillBonus = skillList->createWidget<MyGUI::StaticText>("SandTextRight", coord2, MyGUI::Align::Default, bonus.str());
		skillBonus->setCaption(inputList[i][1]);

		skillItems.push_back(skillName);
		skillItems.push_back(skillBonus);

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

	MyGUI::StaticTextPtr spellPowerName;
	const int lineHeight = 18;
	MyGUI::IntCoord coord(0, 0, spellPowerList->getWidth(), 18);

	const char *inputList[] = {
		"Depth Perception",
		"Resist Fire",
		"Ancestor Guardian",
		0
	};

	for (int i = 0; inputList[i]; ++i)
	{
		std::ostringstream name;
		name << std::string("SpellPowerName") << i;
		spellPowerName = spellPowerList->createWidget<MyGUI::StaticText>("SandText", coord, MyGUI::Align::Default, name.str());
		spellPowerName->setCaption(inputList[i]);

		spellPowerItems.push_back(spellPowerName);

		coord.top += lineHeight;
	}
}
