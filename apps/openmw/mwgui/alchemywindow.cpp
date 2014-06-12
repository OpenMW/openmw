#include "alchemywindow.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "itemview.hpp"
#include "itemwidget.hpp"

namespace MWGui
{
    AlchemyWindow::AlchemyWindow()
        : WindowBase("openmw_alchemy_window.layout")
        , mApparatus (4)
        , mIngredients (4)
        , mSortModel(NULL)
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
        std::string name = mNameEdit->getCaption();
        boost::algorithm::trim(name);

        MWMechanics::Alchemy::Result result = mAlchemy.create (mNameEdit->getCaption ());

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
        mAlchemy.setAlchemist (MWBase::Environment::get().getWorld()->getPlayerPtr());

        InventoryItemModel* model = new InventoryItemModel(MWBase::Environment::get().getWorld()->getPlayerPtr());
        mSortModel = new SortFilterItemModel(model);
        mSortModel->setFilter(SortFilterItemModel::Filter_OnlyIngredients);
        mItemView->setModel (mSortModel);

        mNameEdit->setCaption("");

        int index = 0;

        mAlchemy.setAlchemist (MWBase::Environment::get().getWorld()->getPlayerPtr());

        for (MWMechanics::Alchemy::TToolsIterator iter (mAlchemy.beginTools());
            iter!=mAlchemy.endTools() && index<static_cast<int> (mApparatus.size()); ++iter, ++index)
        {
            if (!iter->isEmpty())
            {
                mApparatus.at (index)->setUserString ("ToolTipType", "ItemPtr");
                mApparatus.at (index)->setUserData (*iter);
                mApparatus.at (index)->setItem(*iter);
            }
        }

        update();
    }

    void AlchemyWindow::exit() {
        mAlchemy.clear();
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
        int res = mAlchemy.addIngredient(item);

        if (res != -1)
        {
            update();

            std::string sound = item.getClass().getUpSoundId(item);
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
        }
    }

    void AlchemyWindow::update()
    {
        mSortModel->clearDragItems();

        MWMechanics::Alchemy::TIngredientsIterator it = mAlchemy.beginIngredients ();
        for (int i=0; i<4; ++i)
        {
            ItemWidget* ingredient = mIngredients[i];

            MWWorld::Ptr item;
            if (it != mAlchemy.endIngredients ())
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

            MyGUI::TextBox* text = ingredient->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
            text->setTextAlign(MyGUI::Align::Right);
            text->setNeedMouseFocus(false);
            text->setTextShadow(true);
            text->setTextShadowColour(MyGUI::Colour(0,0,0));
            text->setCaption(ItemView::getCountString(ingredient->getUserData<MWWorld::Ptr>()->getRefData().getCount()));
        }

        mItemView->update();

        std::vector<ESM::ENAMstruct> effects;
        ESM::EffectList list;
        list.mList = effects;
        for (MWMechanics::Alchemy::TEffectsIterator it = mAlchemy.beginEffects (); it != mAlchemy.endEffects (); ++it)
        {
            list.mList.push_back(*it);
        }

        while (mEffectsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEffectsBox->getChildAt(0));

        MyGUI::IntCoord coord(0, 0, mEffectsBox->getWidth(), 24);
        Widgets::MWEffectListPtr effectsWidget = mEffectsBox->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, MyGUI::Align::Left | MyGUI::Align::Top);

        Widgets::SpellEffectList _list = Widgets::MWEffectList::effectListFromESM(&list);
        effectsWidget->setEffectList(_list);

        std::vector<MyGUI::Widget*> effectItems;
        effectsWidget->createEffectWidgets(effectItems, mEffectsBox, coord, false, 0);
        effectsWidget->setCoord(coord);
    }

    void AlchemyWindow::removeIngredient(MyGUI::Widget* ingredient)
    {
        for (int i=0; i<4; ++i)
            if (mIngredients[i] == ingredient)
                mAlchemy.removeIngredient (i);

        update();
    }
}
