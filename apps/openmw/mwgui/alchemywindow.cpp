#include "alchemywindow.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
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
#include "widgets.hpp"

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

        mNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &AlchemyWindow::onAccept);

        center();
    }

    void AlchemyWindow::onAccept(MyGUI::EditBox* sender)
    {
        onCreateButtonClicked(sender);

        // To do not spam onAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void AlchemyWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Alchemy);
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
        MWMechanics::Alchemy::Result result = mAlchemy->create (mNameEdit->getCaption ());
        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();

        switch (result)
        {
        case MWMechanics::Alchemy::Result_NoName:
            winMgr->messageBox("#{sNotifyMessage37}");
            break;
        case MWMechanics::Alchemy::Result_NoMortarAndPestle:
            winMgr->messageBox("#{sNotifyMessage45}");
            break;
        case MWMechanics::Alchemy::Result_LessThanTwoIngredients:
            winMgr->messageBox("#{sNotifyMessage6a}");
            break;
        case MWMechanics::Alchemy::Result_Success:
            winMgr->messageBox("#{sPotionSuccess}");
            winMgr->playSound("potion success");
            break;
        case MWMechanics::Alchemy::Result_NoEffects:
        case MWMechanics::Alchemy::Result_RandomFailure:
            winMgr->messageBox("#{sNotifyMessage8}");
            winMgr->playSound("potion fail");
            break;
        }

        // remove ingredient slots that have been fully used up
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
            {
                MWWorld::Ptr ingred = *mIngredients[i]->getUserData<MWWorld::Ptr>();
                if (ingred.getRefData().getCount() == 0)
                    removeIngredient(mIngredients[i]);
            }

        update();
    }

    void AlchemyWindow::onOpen()
    {
        mAlchemy->clear();
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
                mApparatus.at (index)->setUserData (MWWorld::Ptr(*iter));
            }
        }

        update();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mNameEdit);
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
            MWBase::Environment::get().getWindowManager()->playSound(sound);
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
            ingredient->setUserData(MWWorld::Ptr(item));

            ingredient->setCount(item.getRefData().getCount());
        }

        mItemView->update();

        std::set<MWMechanics::EffectKey> effectIds = mAlchemy->listEffects();
        Widgets::SpellEffectList list;
        unsigned int effectIndex=0;
        for (std::set<MWMechanics::EffectKey>::iterator it2 = effectIds.begin(); it2 != effectIds.end(); ++it2)
        {
            Widgets::SpellEffectParams params;
            params.mEffectID = it2->mId;
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(it2->mId);
            if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                params.mSkill = it2->mArg;
            else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                params.mAttribute = it2->mArg;
            params.mIsConstant = true;
            params.mNoTarget = true;

            params.mKnown = mAlchemy->knownEffect(effectIndex, MWBase::Environment::get().getWorld()->getPlayerPtr());

            list.push_back(params);
            ++effectIndex;
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
