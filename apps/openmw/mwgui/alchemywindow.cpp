#include "alchemywindow.hpp"

#include <boost/algorithm/string.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/containerstore.hpp"

namespace
{
    std::string getIconPath(MWWorld::Ptr ptr)
    {
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(ptr).getInventoryIcon(ptr);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        return path;
    }
}

namespace MWGui
{
    AlchemyWindow::AlchemyWindow(MWBase::WindowManager& parWindowManager)
        : WindowBase("openmw_alchemy_window.layout", parWindowManager)
        , ContainerBase(0), mApparatus (4), mIngredients (4)
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

        mIngredients[0]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[1]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[2]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[3]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);

        mCreateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCreateButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCancelButtonClicked);

        MyGUI::ScrollView* itemView;
        MyGUI::Widget* containerWidget;
        getWidget(containerWidget, "Items");
        getWidget(itemView, "ItemView");
        setWidgets(containerWidget, itemView);

        center();
    }

    void AlchemyWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mAlchemy.clear();

        mWindowManager.removeGuiMode(GM_Alchemy);
        mWindowManager.removeGuiMode(GM_Inventory);
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
        std::string name = mNameEdit->getCaption();
        boost::algorithm::trim(name);

        MWMechanics::Alchemy::Result result = mAlchemy.create (mNameEdit->getCaption ());

        if (result == MWMechanics::Alchemy::Result_NoName)
        {
            mWindowManager.messageBox("#{sNotifyMessage37}", std::vector<std::string>());
            return;
        }

        // check if mortar & pestle is available (always needed)
        if (result == MWMechanics::Alchemy::Result_NoMortarAndPestle)
        {
            mWindowManager.messageBox("#{sNotifyMessage45}", std::vector<std::string>());
            return;
        }

        // make sure 2 or more ingredients were selected
        if (result == MWMechanics::Alchemy::Result_LessThanTwoIngredients)
        {
            mWindowManager.messageBox("#{sNotifyMessage6a}", std::vector<std::string>());
            return;
        }

        if (result == MWMechanics::Alchemy::Result_NoEffects)
        {
            mWindowManager.messageBox("#{sNotifyMessage8}", std::vector<std::string>());
            MWBase::Environment::get().getSoundManager()->playSound("potion fail", 1.f, 1.f);
            return;
        }

        if (result == MWMechanics::Alchemy::Result_Success)
        {
            mWindowManager.messageBox("#{sPotionSuccess}", std::vector<std::string>());
            MWBase::Environment::get().getSoundManager()->playSound("potion success", 1.f, 1.f);
        }
        else if (result == MWMechanics::Alchemy::Result_RandomFailure)
        {
            // potion failed
            mWindowManager.messageBox("#{sNotifyMessage8}", std::vector<std::string>());
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
        openContainer (MWBase::Environment::get().getWorld()->getPlayer().getPlayer()); // this sets mPtr
        setFilter (ContainerBase::Filter_Ingredients);

        mNameEdit->setCaption("");

        mAlchemy.setAlchemist (mPtr);

        int index = 0;

        for (MWMechanics::Alchemy::TToolsIterator iter (mAlchemy.beginTools());
            iter!=mAlchemy.endTools() && index<static_cast<int> (mApparatus.size()); ++iter, ++index)
        {
            if (!iter->isEmpty())
            {
                mApparatus.at (index)->setUserString ("ToolTipType", "ItemPtr");
                mApparatus.at (index)->setUserData (*iter);
                mApparatus.at (index)->setImageTexture (getIconPath (*iter));
            }
        }

        update();
    }

    void AlchemyWindow::onIngredientSelected(MyGUI::Widget* _sender)
    {
        removeIngredient(_sender);
        update();
    }

    void AlchemyWindow::onSelectedItemImpl(MWWorld::Ptr item)
    {
        int res = mAlchemy.addIngredient(item);

        if (res != -1)
        {
            update();

            std::string sound = MWWorld::Class::get(item).getUpSoundId(item);
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
        }
    }

    std::vector<MWWorld::Ptr> AlchemyWindow::itemsToIgnore()
    {
        std::vector<MWWorld::Ptr> ignore;
        // don't show ingredients that are currently selected in the "available ingredients" box.
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
                ignore.push_back(*mIngredients[i]->getUserData<MWWorld::Ptr>());

        return ignore;
    }

    void AlchemyWindow::update()
    {
        MWMechanics::Alchemy::TIngredientsIterator it = mAlchemy.beginIngredients ();
        for (int i=0; i<4; ++i)
        {
            MyGUI::ImageBox* ingredient = mIngredients[i];

            MWWorld::Ptr item;
            if (it != mAlchemy.endIngredients ())
            {
                item = *it;
                ++it;
            }

            if (ingredient->getChildCount())
                MyGUI::Gui::getInstance().destroyWidget(ingredient->getChildAt(0));

            ingredient->setImageTexture("");
            ingredient->clearUserStrings ();

            if (item.isEmpty ())
                continue;

            ingredient->setUserString("ToolTipType", "ItemPtr");
            ingredient->setUserData(item);
            ingredient->setImageTexture(getIconPath(item));

            MyGUI::TextBox* text = ingredient->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
            text->setTextAlign(MyGUI::Align::Right);
            text->setNeedMouseFocus(false);
            text->setTextShadow(true);
            text->setTextShadowColour(MyGUI::Colour(0,0,0));
            text->setCaption(getCountString(ingredient->getUserData<MWWorld::Ptr>()->getRefData().getCount()));
        }

        drawItems();

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
        effectsWidget->setWindowManager(&mWindowManager);

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
