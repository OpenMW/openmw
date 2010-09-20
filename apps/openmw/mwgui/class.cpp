#include "class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;

PickClassDialog::PickClassDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_class_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;
    setText("SpecializationT", wm->getGameSettingString("sChooseClassMenu1", "Specialization"));
    getWidget(specializationName, "SpecializationName");

    setText("FavoriteAttributesT", wm->getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
    getWidget(favoriteAttribute0, "FavoriteAttribute0");
    getWidget(favoriteAttribute1, "FavoriteAttribute1");
    favoriteAttribute0->setWindowManager(wm);
    favoriteAttribute1->setWindowManager(wm);

    setText("MajorSkillT", wm->getGameSettingString("sChooseClassMenu3", "Major Skills:"));
    getWidget(majorSkill0, "MajorSkill0");
    getWidget(majorSkill1, "MajorSkill1");
    getWidget(majorSkill2, "MajorSkill2");
    getWidget(majorSkill3, "MajorSkill3");
    getWidget(majorSkill4, "MajorSkill4");
    majorSkill0->setWindowManager(wm);
    majorSkill1->setWindowManager(wm);
    majorSkill2->setWindowManager(wm);
    majorSkill3->setWindowManager(wm);
    majorSkill4->setWindowManager(wm);

    setText("MinorSkillT", wm->getGameSettingString("sChooseClassMenu4", "Minor Skills:"));
    getWidget(minorSkill0, "MinorSkill0");
    getWidget(minorSkill1, "MinorSkill1");
    getWidget(minorSkill2, "MinorSkill2");
    getWidget(minorSkill3, "MinorSkill3");
    getWidget(minorSkill4, "MinorSkill4");
    minorSkill0->setWindowManager(wm);
    minorSkill1->setWindowManager(wm);
    minorSkill2->setWindowManager(wm);
    minorSkill3->setWindowManager(wm);
    minorSkill4->setWindowManager(wm);

    getWidget(classList, "ClassList");
    classList->setScrollVisible(true);
    classList->eventListSelectAccept = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListMouseItemActivate = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListChangePosition = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);

    getWidget(classImage, "ClassImage");

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

    updateClasses();
    updateStats();
}

void PickClassDialog::setNextButtonShow(bool shown)
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
        backButton->setCoord(MyGUI::IntCoord(382 - 18, 265, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(434 - 18, 265, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        backButton->setCoord(MyGUI::IntCoord(382, 265, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(434, 265, 42, 23));
    }
}

void PickClassDialog::open()
{
    updateClasses();
    updateStats();
    setVisible(true);
}


void PickClassDialog::setClassId(const std::string &classId)
{
    currentClassId = classId;
    classList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = classList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*classList->getItemDataAt<std::string>(i), classId))
        {
            classList->setIndexSelected(i);
            break;
        }
    }

    updateStats();
}

// widget controls

void PickClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void PickClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void PickClassDialog::onSelectClass(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    const std::string *classId = classList->getItemDataAt<std::string>(_index);
    if (boost::iequals(currentClassId, *classId))
        return;

    currentClassId = *classId;
    updateStats();
}

// update widget content

void PickClassDialog::updateClasses()
{
    classList->removeAllItems();

    ESMS::ESMStore &store = environment.mWorld->getStore();
    
    ESMS::RecListT<ESM::Class>::MapType::const_iterator it = store.classes.list.begin();
    ESMS::RecListT<ESM::Class>::MapType::const_iterator end = store.classes.list.end();
    int index = 0;
    for (; it != end; ++it)
    {
        const ESM::Class &klass = it->second;
        bool playable = (klass.data.isPlayable != 0);
        if (!playable) // Only display playable classes
            continue;

        const std::string &id = it->first;
        classList->addItem(klass.name, id);
        if (boost::iequals(id, currentClassId))
            classList->setIndexSelected(index);
        ++index;
    }
}

void PickClassDialog::updateStats()
{
    if (currentClassId.empty())
        return;
    WindowManager *wm = environment.mWindowManager;
    ESMS::ESMStore &store = environment.mWorld->getStore();
    const ESM::Class *klass = store.classes.find(currentClassId);

    ESM::Class::Specialization specialization = static_cast<ESM::Class::Specialization>(klass->data.specialization);

    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    specializationName->setCaption(wm->getGameSettingString(specIds[specialization], specIds[specialization]));

    favoriteAttribute0->setAttributeId(klass->data.attribute[0]);
    favoriteAttribute1->setAttributeId(klass->data.attribute[1]);

    Widgets::MWSkillPtr majorSkills[5] = {
        majorSkill0,
        majorSkill1,
        majorSkill2,
        majorSkill3,
        majorSkill4
    };
    Widgets::MWSkillPtr minorSkills[5] = {
        minorSkill0,
        minorSkill1,
        minorSkill2,
        minorSkill3,
        minorSkill4
    };

    for (int i = 0; i < 5; ++i)
    {
        majorSkills[i]->setSkillNumber(klass->data.skills[i][0]);
        minorSkills[i]->setSkillNumber(klass->data.skills[i][1]);
    }

    classImage->setImageTexture(std::string("textures\\levelup\\") + currentClassId + ".dds");
}
