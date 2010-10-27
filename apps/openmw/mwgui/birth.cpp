#include "birth.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

BirthDialog::BirthDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_birth_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;

    getWidget(spellArea, "SpellArea");

    getWidget(birthImage, "BirthsignImage");

    getWidget(birthList, "BirthsignList");
    birthList->setScrollVisible(true);
    birthList->eventListSelectAccept = MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);
    birthList->eventListMouseItemActivate = MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);
    birthList->eventListChangePosition = MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &BirthDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &BirthDialog::onOkClicked);

    updateBirths();
    updateSpells();
}

void BirthDialog::setNextButtonShow(bool shown)
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
        backButton->setCoord(MyGUI::IntCoord(375 - 18, 340, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(431 - 18, 340, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        backButton->setCoord(MyGUI::IntCoord(375, 340, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(431, 340, 42, 23));
    }
}

void BirthDialog::open()
{
    updateBirths();
    updateSpells();
    setVisible(true);
}


void BirthDialog::setBirthId(const std::string &birthId)
{
    currentBirthId = birthId;
    birthList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = birthList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*birthList->getItemDataAt<std::string>(i), birthId))
        {
            birthList->setIndexSelected(i);
            break;
        }
    }

    updateSpells();
}

// widget controls

void BirthDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void BirthDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void BirthDialog::onSelectBirth(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    const std::string *birthId = birthList->getItemDataAt<std::string>(_index);
    if (boost::iequals(currentBirthId, *birthId))
        return;

    currentBirthId = *birthId;
    updateSpells();
}

// update widget content

void BirthDialog::updateBirths()
{
    birthList->removeAllItems();

    ESMS::ESMStore &store = environment.mWorld->getStore();
    
    ESMS::RecListT<ESM::BirthSign>::MapType::const_iterator it = store.birthSigns.list.begin();
    ESMS::RecListT<ESM::BirthSign>::MapType::const_iterator end = store.birthSigns.list.end();
    int index = 0;
    for (; it != end; ++it)
    {
        const ESM::BirthSign &birth = it->second;
        birthList->addItem(birth.name, it->first);
        if (boost::iequals(it->first, currentBirthId))
            birthList->setIndexSelected(index);
        ++index;
    }
}

void BirthDialog::updateSpells()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = spellItems.begin(); it != spellItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    spellItems.clear();

    if (currentBirthId.empty())
        return;

    MWSpellPtr spellWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord(0, 0, spellArea->getWidth(), 18);

    ESMS::ESMStore &store = environment.mWorld->getStore();
    const ESM::BirthSign *birth = store.birthSigns.find(currentBirthId);

    std::string texturePath = std::string("textures\\") + birth->texture;
    fixTexturePath(texturePath);
    birthImage->setImageTexture(texturePath);

    std::vector<std::string> abilities, powers, spells;

    std::vector<std::string>::const_iterator it = birth->powers.list.begin();
    std::vector<std::string>::const_iterator end = birth->powers.list.end();
    for (; it != end; ++it)
    {
        const std::string &spellId = *it;
        const ESM::Spell *spell = store.spells.search(spellId);
        if (!spell)
            continue; // Skip spells which cannot be found
        ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->data.type);
        if (type != ESM::Spell::ST_Spell && type != ESM::Spell::ST_Ability && type != ESM::Spell::ST_Power)
            continue; // We only want spell, ability and powers.

        if (type == ESM::Spell::ST_Ability)
            abilities.push_back(spellId);
        else if (type == ESM::Spell::ST_Power)
            powers.push_back(spellId);
        else if (type == ESM::Spell::ST_Spell)
            spells.push_back(spellId);
    }

    int i = 0;
    struct{ const std::vector<std::string> &spells; const char *label; } categories[3] = {
        {abilities, "sBirthsignmenu1"},
        {powers,    "sPowers"},
        {spells,    "sBirthsignmenu2"}
    };
    for (int category = 0; category < 3; ++category)
    {
        if (!categories[category].spells.empty())
        {
            MyGUI::StaticTextPtr label = spellArea->createWidget<MyGUI::StaticText>("SandBrightText", coord, MyGUI::Align::Default, std::string("Label"));
            label->setCaption(environment.mWindowManager->getGameSettingString(categories[category].label, ""));
            spellItems.push_back(label);
            coord.top += lineHeight;

            std::vector<std::string>::const_iterator end = categories[category].spells.end();
            for (std::vector<std::string>::const_iterator it = categories[category].spells.begin(); it != end; ++it)
            {
                const std::string &spellId = *it;
                spellWidget = spellArea->createWidget<MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("Spell") + boost::lexical_cast<std::string>(i));
                spellWidget->setEnvironment(&environment);
                spellWidget->setSpellId(spellId);

                spellItems.push_back(spellWidget);
                coord.top += lineHeight;

                MyGUI::IntCoord spellCoord = coord;
                spellCoord.height = 24; // TODO: This should be fetched from the skin somehow, or perhaps a widget in the layout as a template?
                spellWidget->createEffectWidgets(spellItems, spellArea, spellCoord);
                coord.top = spellCoord.top;

                ++i;
            }
        }
    }
}
