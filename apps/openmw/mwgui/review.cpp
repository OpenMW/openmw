#include "review.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

ReviewDialog::ReviewDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
    : Layout("openmw_chargen_review_layout.xml")
    , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;

    // Setup static stats
    StaticTextPtr name, race, klass, sign;
    ButtonPtr button;
    getWidget(name, "NameText");
    name->setCaption("Drizt");
    getWidget(button, "NameButton");
    button->setCaption(wm->getGameSettingString("sName", ""));

    getWidget(race, "RaceText");
    race->setCaption("Dark Elf");
    getWidget(button, "RaceButton");
    button->setCaption(wm->getGameSettingString("sRace", ""));

    getWidget(klass, "ClassText");
    klass->setCaption("Adventurer");
    getWidget(button, "ClassButton");
    button->setCaption(wm->getGameSettingString("sClass", ""));

    getWidget(sign, "SignText");
    sign->setCaption("The Angel");
    getWidget(button, "SignButton");
    button->setCaption(wm->getGameSettingString("sBirthSign", ""));

    // Setup dynamic stats
    MWDynamicStatPtr health, magicka, fatigue;
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
        attribute->setWindowManager(wm);
        attribute->setAttributeId(ESM::Attribute::attributeIds[idx]);
        attribute->setAttributeValue(MWAttribute::AttributeValue(40, 50));
    }

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &ReviewDialog::onOkClicked);
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
