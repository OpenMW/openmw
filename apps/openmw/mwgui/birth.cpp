#include "birth.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "widgets.hpp"

using namespace MWGui;
using namespace Widgets;

namespace
{

bool sortBirthSigns(const std::pair<std::string, const ESM::BirthSign*>& left, const std::pair<std::string, const ESM::BirthSign*>& right)
{
    return left.second->mName.compare (right.second->mName) < 0;
}

}

BirthDialog::BirthDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_birth.layout", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(mSpellArea, "SpellArea");

    getWidget(mBirthImage, "BirthsignImage");

    getWidget(mBirthList, "BirthsignList");
    mBirthList->setScrollVisible(true);
    mBirthList->eventListSelectAccept += MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);
    mBirthList->eventListMouseItemActivate += MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);
    mBirthList->eventListChangePosition += MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);

    MyGUI::Button* backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BirthDialog::onBackClicked);

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BirthDialog::onOkClicked);

    updateBirths();
    updateSpells();
}

void BirthDialog::setNextButtonShow(bool shown)
{
    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
}

void BirthDialog::open()
{
    WindowModal::open();
    updateBirths();
    updateSpells();
}


void BirthDialog::setBirthId(const std::string &birthId)
{
    mCurrentBirthId = birthId;
    mBirthList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = mBirthList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*mBirthList->getItemDataAt<std::string>(i), birthId))
        {
            mBirthList->setIndexSelected(i);
            MyGUI::Button* okButton;
            getWidget(okButton, "OKButton");
            break;
        }
    }

    updateSpells();
}

// widget controls

void BirthDialog::onOkClicked(MyGUI::Widget* _sender)
{
    if(mBirthList->getIndexSelected() == MyGUI::ITEM_NONE)
        return;
    eventDone(this);
}

void BirthDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void BirthDialog::onSelectBirth(MyGUI::ListBox* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    const std::string *birthId = mBirthList->getItemDataAt<std::string>(_index);
    if (boost::iequals(mCurrentBirthId, *birthId))
        return;

    mCurrentBirthId = *birthId;
    updateSpells();
}

// update widget content

void BirthDialog::updateBirths()
{
    mBirthList->removeAllItems();

    const MWWorld::Store<ESM::BirthSign> &signs =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>();

    // sort by name
    std::vector < std::pair<std::string, const ESM::BirthSign*> > birthSigns;

    MWWorld::Store<ESM::BirthSign>::iterator it = signs.begin();
    for (; it != signs.end(); ++it)
    {
        birthSigns.push_back(std::make_pair(it->mId, &(*it)));
    }
    std::sort(birthSigns.begin(), birthSigns.end(), sortBirthSigns);

    int index = 0;
    for (std::vector<std::pair<std::string, const ESM::BirthSign*> >::const_iterator it2 = birthSigns.begin();
         it2 != birthSigns.end(); ++it2, ++index)
    {
        mBirthList->addItem(it2->second->mName, it2->first);
        if (mCurrentBirthId.empty())
        {
            mBirthList->setIndexSelected(index);
            mCurrentBirthId = it2->first;
        }
        else if (boost::iequals(it2->first, mCurrentBirthId))
        {
            mBirthList->setIndexSelected(index);
        }
    }
}

void BirthDialog::updateSpells()
{
    for (std::vector<MyGUI::Widget*>::iterator it = mSpellItems.begin(); it != mSpellItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    mSpellItems.clear();

    if (mCurrentBirthId.empty())
        return;

    MWSpellPtr spellWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord(0, 0, mSpellArea->getWidth(), 18);

    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();

    const ESM::BirthSign *birth =
        store.get<ESM::BirthSign>().find(mCurrentBirthId);

    std::string texturePath = std::string("textures\\") + birth->mTexture;
    fixTexturePath(texturePath);
    mBirthImage->setImageTexture(texturePath);

    std::vector<std::string> abilities, powers, spells;

    std::vector<std::string>::const_iterator it = birth->mPowers.mList.begin();
    std::vector<std::string>::const_iterator end = birth->mPowers.mList.end();
    for (; it != end; ++it)
    {
        const std::string &spellId = *it;
        const ESM::Spell *spell = store.get<ESM::Spell>().search(spellId);
        if (!spell)
            continue; // Skip spells which cannot be found
        ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->mData.mType);
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

    struct {
        const std::vector<std::string> &spells;
        const char *label;
    }
    categories[3] = {
        {abilities, "sBirthsignmenu1"},
        {powers,    "sPowers"},
        {spells,    "sBirthsignmenu2"}
    };

    for (int category = 0; category < 3; ++category)
    {
        if (!categories[category].spells.empty())
        {
            MyGUI::TextBox* label = mSpellArea->createWidget<MyGUI::TextBox>("SandBrightText", coord, MyGUI::Align::Default, std::string("Label"));
            label->setCaption(mWindowManager.getGameSettingString(categories[category].label, ""));
            mSpellItems.push_back(label);
            coord.top += lineHeight;

            std::vector<std::string>::const_iterator end = categories[category].spells.end();
            for (std::vector<std::string>::const_iterator it = categories[category].spells.begin(); it != end; ++it)
            {
                const std::string &spellId = *it;
                spellWidget = mSpellArea->createWidget<MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("Spell") + boost::lexical_cast<std::string>(i));
                spellWidget->setWindowManager(&mWindowManager);
                spellWidget->setSpellId(spellId);

                mSpellItems.push_back(spellWidget);
                coord.top += lineHeight;

                MyGUI::IntCoord spellCoord = coord;
                spellCoord.height = 24; // TODO: This should be fetched from the skin somehow, or perhaps a widget in the layout as a template?
                spellWidget->createEffectWidgets(mSpellItems, mSpellArea, spellCoord, (category == 0) ? MWEffectList::EF_Constant : 0);
                coord.top = spellCoord.top;

                ++i;
            }
        }
    }
}
