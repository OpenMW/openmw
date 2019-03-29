#include "birth.hpp"

#include <MyGUI_ListBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ScrollView.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "widgets.hpp"

namespace
{

    bool sortBirthSigns(const std::pair<std::string, const ESM::BirthSign*>& left, const std::pair<std::string, const ESM::BirthSign*>& right)
    {
        return left.second->mName.compare (right.second->mName) < 0;
    }

}

namespace MWGui
{

    BirthDialog::BirthDialog()
      : WindowModal("openmw_chargen_birth.layout")
    {
        // Centre dialog
        center();

        getWidget(mSpellArea, "SpellArea");

        getWidget(mBirthImage, "BirthsignImage");

        getWidget(mBirthList, "BirthsignList");
        mBirthList->setScrollVisible(true);
        mBirthList->eventListSelectAccept += MyGUI::newDelegate(this, &BirthDialog::onAccept);
        mBirthList->eventListChangePosition += MyGUI::newDelegate(this, &BirthDialog::onSelectBirth);

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BirthDialog::onBackClicked);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &BirthDialog::onOkClicked);

        updateBirths();
        updateSpells();
    }

    void BirthDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    void BirthDialog::onOpen()
    {
        WindowModal::onOpen();
        updateBirths();
        updateSpells();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mBirthList);

        // Show the current birthsign by default
        const std::string &signId =
            MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();

        if (!signId.empty())
            setBirthId(signId);
    }

    void BirthDialog::setBirthId(const std::string &birthId)
    {
        mCurrentBirthId = birthId;
        mBirthList->setIndexSelected(MyGUI::ITEM_NONE);
        size_t count = mBirthList->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (Misc::StringUtils::ciEqual(*mBirthList->getItemDataAt<std::string>(i), birthId))
            {
                mBirthList->setIndexSelected(i);
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

    void BirthDialog::onAccept(MyGUI::ListBox *_sender, size_t _index)
    {
        onSelectBirth(_sender, _index);
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

        const std::string *birthId = mBirthList->getItemDataAt<std::string>(_index);
        if (Misc::StringUtils::ciEqual(mCurrentBirthId, *birthId))
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

        for (const ESM::BirthSign& sign : signs)
        {
            birthSigns.push_back(std::make_pair(sign.mId, &sign));
        }
        std::sort(birthSigns.begin(), birthSigns.end(), sortBirthSigns);

        int index = 0;
        for (auto& birthsignPair : birthSigns)
        {
            mBirthList->addItem(birthsignPair.second->mName, birthsignPair.first);
            if (mCurrentBirthId.empty())
            {
                mBirthList->setIndexSelected(index);
                mCurrentBirthId = birthsignPair.first;
            }
            else if (Misc::StringUtils::ciEqual(birthsignPair.first, mCurrentBirthId))
            {
                mBirthList->setIndexSelected(index);
            }

            index++;
        }
    }

    void BirthDialog::updateSpells()
    {
        for (MyGUI::Widget* widget : mSpellItems)
        {
            MyGUI::Gui::getInstance().destroyWidget(widget);
        }
        mSpellItems.clear();

        if (mCurrentBirthId.empty())
            return;

        Widgets::MWSpellPtr spellWidget;
        const int lineHeight = 18;
        MyGUI::IntCoord coord(0, 0, mSpellArea->getWidth(), 18);

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::BirthSign *birth =
            store.get<ESM::BirthSign>().find(mCurrentBirthId);

        mBirthImage->setImageTexture(MWBase::Environment::get().getWindowManager()->correctTexturePath(birth->mTexture));

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
                label->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString(categories[category].label, ""));
                mSpellItems.push_back(label);
                coord.top += lineHeight;

                end = categories[category].spells.end();
                for (it = categories[category].spells.begin(); it != end; ++it)
                {
                    const std::string &spellId = *it;
                    spellWidget = mSpellArea->createWidget<Widgets::MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("Spell") + MyGUI::utility::toString(i));
                    spellWidget->setSpellId(spellId);

                    mSpellItems.push_back(spellWidget);
                    coord.top += lineHeight;

                    MyGUI::IntCoord spellCoord = coord;
                    spellCoord.height = 24; // TODO: This should be fetched from the skin somehow, or perhaps a widget in the layout as a template?
                    spellWidget->createEffectWidgets(mSpellItems, mSpellArea, spellCoord, (category == 0) ? Widgets::MWEffectList::EF_Constant : 0);
                    coord.top = spellCoord.top;

                    ++i;
                }
            }
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSpellArea->setVisibleVScroll(false);
        mSpellArea->setCanvasSize(MyGUI::IntSize(mSpellArea->getWidth(), std::max(mSpellArea->getHeight(), coord.top)));
        mSpellArea->setVisibleVScroll(true);
        mSpellArea->setViewOffset(MyGUI::IntPoint(0, 0));
    }
}
