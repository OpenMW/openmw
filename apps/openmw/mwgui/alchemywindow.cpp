#include "alchemywindow.hpp"

#include <MyGUI_Gui.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/alchemy.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include <components/esm/records.hpp>

#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "itemview.hpp"
#include "itemwidget.hpp"

namespace MWGui
{
    AlchemyWindow::AlchemyWindow()
        : WindowBase("openmw_alchemy_window.layout")
        , mSortModel(NULL)
        , mAlchemy(new MWMechanics::Alchemy())
        , mApparatus (4)
        , mIngredients (4)
    {
        getWidget(mCreateButton, "CreateButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mIngredients[0], "Ingredient1");
        getWidget(mIngredients[1], "Ingredient2");
        getWidget(mIngredients[2], "Ingredient3");
        getWidget(mIngredients[3], "Ingredient4");
        getWidget(mApparatus[0], "Apparatus1");
        getWidget(mApparatus[1], "Apparatus2");
        getWidget(mApparatus[2], "Apparatus3");
        getWidget(mApparatus[3], "Apparatus4");
        getWidget(mEffectsBox, "CreatedEffects");
        getWidget(mNameEdit, "NameEdit");
        getWidget(mItemView, "ItemView");


        mItemView->eventItemClicked += MyGUI::newDelegate(this, &AlchemyWindow::onSelectedItem);

        mIngredients[0]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[1]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[2]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[3]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);

        mCreateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCreateButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCancelButtonClicked);

        center();
    }

    void AlchemyWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
        MWMechanics::Alchemy::Result result = mAlchemy->create (mNameEdit->getCaption ());

        if (result == MWMechanics::Alchemy::Result_NoName)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage37}");
            return;
        }

        // check if mortar & pestle is available (always needed)
        if (result == MWMechanics::Alchemy::Result_NoMortarAndPestle)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage45}");
            return;
        }

        // make sure 2 or more ingredients were selected
        if (result == MWMechanics::Alchemy::Result_LessThanTwoIngredients)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage6a}");
            return;
        }

        if (result == MWMechanics::Alchemy::Result_NoEffects)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage8}");
            MWBase::Environment::get().getSoundManager()->playSound("potion fail", 1.f, 1.f);
            return;
        }

        if (result == MWMechanics::Alchemy::Result_Success)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sPotionSuccess}");
            MWBase::Environment::get().getSoundManager()->playSound("potion success", 1.f, 1.f);
        }
        else if (result == MWMechanics::Alchemy::Result_RandomFailure)
        {
            // potion failed
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage8}");
            MWBase::Environment::get().getSoundManager()->playSound("potion fail", 1.f, 1.f);
        }

        // reduce count of the ingredients
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
            {
                MWWorld::Ptr ingred = *mIngredients[i]->getUserData<MWWorld::Ptr>();
                if (ingred.getRefData().getCount() == 0)
                    removeIngredient(mIngredients[i]);
            }

        update();
    }

    void AlchemyWindow::open()
    {
        mAlchemy->setAlchemist (MWMechanics::getPlayer());

        InventoryItemModel* model = new InventoryItemModel(MWMechanics::getPlayer());
        mSortModel = new SortFilterItemModel(model);
        mSortModel->setFilter(SortFilterItemModel::Filter_OnlyIngredients);
        mItemView->setModel (mSortModel);
        mItemView->resetScrollBars();

        mNameEdit->setCaption("");

        int index = 0;
        for (MWMechanics::Alchemy::TToolsIterator iter (mAlchemy->beginTools());
            iter!=mAlchemy->endTools() && index<static_cast<int> (mApparatus.size()); ++iter, ++index)
        {
            mApparatus.at (index)->setItem(*iter);
            mApparatus.at (index)->clearUserStrings();
            if (!iter->isEmpty())
            {
                mApparatus.at (index)->setUserString ("ToolTipType", "ItemPtr");
                mApparatus.at (index)->setUserData (*iter);
            }
        }

        update();
    }

    void AlchemyWindow::exit() {
        mAlchemy->clear();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Alchemy);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Inventory);
    }

    void AlchemyWindow::onIngredientSelected(MyGUI::Widget* _sender)
    {
        removeIngredient(_sender);
        update();
    }

    void AlchemyWindow::onSelectedItem(int index)
    {
        MWWorld::Ptr item = mSortModel->getItem(index).mBase;
        int res = mAlchemy->addIngredient(item);

        if (res != -1)
        {
            update();

            std::string sound = item.getClass().getUpSoundId(item);
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
        }
    }

    void AlchemyWindow::update()
    {
        std::string suggestedName = mAlchemy->suggestPotionName();
        if (suggestedName != mSuggestedPotionName)
            mNameEdit->setCaptionWithReplacing(suggestedName);
        mSuggestedPotionName = suggestedName;

        mSortModel->clearDragItems();

        MWMechanics::Alchemy::TIngredientsIterator it = mAlchemy->beginIngredients ();
        for (int i=0; i<4; ++i)
        {
            ItemWidget* ingredient = mIngredients[i];

            MWWorld::Ptr item;
            if (it != mAlchemy->endIngredients ())
            {
                item = *it;
                ++it;
            }

            if (!item.isEmpty())
                mSortModel->addDragItem(item, item.getRefData().getCount());

            if (ingredient->getChildCount())
                MyGUI::Gui::getInstance().destroyWidget(ingredient->getChildAt(0));

            ingredient->clearUserStrings ();

            ingredient->setItem(item);

            if (item.isEmpty ())
                continue;

            ingredient->setUserString("ToolTipType", "ItemPtr");
            ingredient->setUserData(item);

            ingredient->setCount(ingredient->getUserData<MWWorld::Ptr>()->getRefData().getCount());
        }

        mItemView->update();

        std::set<MWMechanics::EffectKey> effectIds = mAlchemy->listEffects();
        Widgets::SpellEffectList list;
        for (std::set<MWMechanics::EffectKey>::iterator it = effectIds.begin(); it != effectIds.end(); ++it)
        {
            Widgets::SpellEffectParams params;
            params.mEffectID = it->mId;
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(it->mId);
            if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                params.mSkill = it->mArg;
            else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                params.mAttribute = it->mArg;
            params.mIsConstant = true;
            params.mNoTarget = true;

            list.push_back(params);
        }

        while (mEffectsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEffectsBox->getChildAt(0));

        MyGUI::IntCoord coord(0, 0, mEffectsBox->getWidth(), 24);
        Widgets::MWEffectListPtr effectsWidget = mEffectsBox->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, MyGUI::Align::Left | MyGUI::Align::Top);

        effectsWidget->setEffectList(list);

        std::vector<MyGUI::Widget*> effectItems;
        effectsWidget->createEffectWidgets(effectItems, mEffectsBox, coord, false, 0);
        effectsWidget->setCoord(coord);
    }

    void AlchemyWindow::removeIngredient(MyGUI::Widget* ingredient)
    {
        for (int i=0; i<4; ++i)
            if (mIngredients[i] == ingredient)
                mAlchemy->removeIngredient (i);

        update();
    }
}
