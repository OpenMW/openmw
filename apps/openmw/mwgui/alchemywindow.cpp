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
        , ContainerBase(0)
    {
        getWidget(mCreateButton, "CreateButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mIngredient1, "Ingredient1");
        getWidget(mIngredient2, "Ingredient2");
        getWidget(mIngredient3, "Ingredient3");
        getWidget(mIngredient4, "Ingredient4");
        getWidget(mApparatus1, "Apparatus1");
        getWidget(mApparatus2, "Apparatus2");
        getWidget(mApparatus3, "Apparatus3");
        getWidget(mApparatus4, "Apparatus4");
        getWidget(mEffectsBox, "CreatedEffects");
        getWidget(mNameEdit, "NameEdit");

        mIngredient1->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredient2->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredient3->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredient4->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);

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
        mWindowManager.removeGuiMode(GM_Alchemy);
        mWindowManager.removeGuiMode(GM_Inventory);
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
        // check if mortar & pestle is available (always needed)
        /// \todo check albemic, calcinator, retort (sometimes needed)
        if (!mApparatus1->isUserString("ToolTipType"))
        {
            mWindowManager.messageBox("#{sNotifyMessage45}", std::vector<std::string>());
            return;
        }

        // make sure 2 or more ingredients were selected
        int numIngreds = 0;
        if (mIngredient1->isUserString("ToolTipType"))
            ++numIngreds;
        if (mIngredient2->isUserString("ToolTipType"))
            ++numIngreds;
        if (mIngredient3->isUserString("ToolTipType"))
            ++numIngreds;
        if (mIngredient4->isUserString("ToolTipType"))
            ++numIngreds;
        if (numIngreds < 2)
        {
            mWindowManager.messageBox("#{sNotifyMessage6a}", std::vector<std::string>());
            return;
        }

        // make sure a name was entered
        std::string name = mNameEdit->getCaption();
        boost::algorithm::trim(name);
        if (name == "")
        {
            mWindowManager.messageBox("#{sNotifyMessage37}", std::vector<std::string>());
            return;
        }

        // if there are no created effects, the potion will always fail (but the ingredients won't be destroyed)
        if (mEffects.empty())
        {
            mWindowManager.messageBox("#{sNotifyMessage8}", std::vector<std::string>());
            MWBase::Environment::get().getSoundManager()->playSound("potion fail", 1.f, 1.f);
            return;
        }

        if (rand() % 2 == 0) /// \todo
        {
            ESM::Potion newPotion;
            newPotion.mName = mNameEdit->getCaption();
            ESM::EffectList effects;
            for (unsigned int i=0; i<4; ++i)
            {
                if (mEffects.size() >= i+1)
                {
                    ESM::ENAMstruct effect;
                    effect.mEffectID = mEffects[i].mEffectID;
                    effect.mArea = 0;
                    effect.mRange = ESM::RT_Self;
                    effect.mSkill = mEffects[i].mSkill;
                    effect.mAttribute = mEffects[i].mAttribute;
                    effect.mMagnMin = 1; /// \todo
                    effect.mMagnMax = 10; /// \todo
                    effect.mDuration = 60; /// \todo
                    effects.mList.push_back(effect);
                }
            }

            // UESP Wiki / Morrowind:Alchemy
            // "The weight of a potion is an average of the weight of the ingredients, rounded down."
            // note by scrawl: not rounding down here, I can't imagine a created potion to
            // have 0 weight when using ingredients with 0.1 weight respectively
            float weight = 0;
            if (mIngredient1->isUserString("ToolTipType"))
                weight += mIngredient1->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>()->base->mData.mWeight;
            if (mIngredient2->isUserString("ToolTipType"))
                weight += mIngredient2->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>()->base->mData.mWeight;
            if (mIngredient3->isUserString("ToolTipType"))
                weight += mIngredient3->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>()->base->mData.mWeight;
            if (mIngredient4->isUserString("ToolTipType"))
                weight += mIngredient4->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>()->base->mData.mWeight;
            newPotion.mData.mWeight = weight / float(numIngreds);

            newPotion.mData.mValue = 100; /// \todo
            newPotion.mEffects = effects;
            // pick a random mesh and icon
            std::vector<std::string> names;
            /// \todo is the mesh/icon dependent on alchemy skill?
            names.push_back("standard");
            names.push_back("bargain");
            names.push_back("cheap");
            names.push_back("fresh");
            names.push_back("exclusive");
            names.push_back("quality");
            int random = rand() % names.size();
            newPotion.mModel = "m\\misc_potion_" + names[random ] + "_01.nif";
            newPotion.mIcon = "m\\tx_potion_" + names[random ] + "_01.dds";

            // check if a similiar potion record exists already
            bool found = false;
            std::string objectId;
            typedef std::map<std::string, ESM::Potion> PotionMap;
            PotionMap potions = MWBase::Environment::get().getWorld()->getStore().potions.list;
            for (PotionMap::const_iterator it = potions.begin(); it != potions.end(); ++it)
            {
                if (found) break;

                if (it->second.mData.mValue == newPotion.mData.mValue
                    && it->second.mData.mWeight == newPotion.mData.mWeight
                    && it->second.mName == newPotion.mName
                    && it->second.mEffects.mList.size() == newPotion.mEffects.mList.size())
                {
                    // check effects
                    for (unsigned int i=0; i < it->second.mEffects.mList.size(); ++i)
                    {
                        const ESM::ENAMstruct& a = it->second.mEffects.mList[i];
                        const ESM::ENAMstruct& b = newPotion.mEffects.mList[i];
                        if (a.mEffectID == b.mEffectID
                            && a.mArea == b.mArea
                            && a.mRange == b.mRange
                            && a.mSkill == b.mSkill
                            && a.mAttribute == b.mAttribute
                            && a.mMagnMin == b.mMagnMin
                            && a.mMagnMax == b.mMagnMax
                            && a.mDuration == b.mDuration)
                        {
                            found = true;
                            objectId = it->first;
                            break;
                        }
                    }
                }
            }

            if (!found)
            {
                std::pair<std::string, const ESM::Potion*> result = MWBase::Environment::get().getWorld()->createRecord(newPotion);
                objectId = result.first;
            }

            // create a reference and add it to player inventory
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), objectId);
            MWWorld::ContainerStore& store = MWWorld::Class::get(mPtr).getContainerStore(mPtr);
            ref.getPtr().getRefData().setCount(1);
            store.add(ref.getPtr());

            mWindowManager.messageBox("#{sPotionSuccess}", std::vector<std::string>());
            MWBase::Environment::get().getSoundManager()->playSound("potion success", 1.f, 1.f);
        }
        else
        {
            // potion failed
            mWindowManager.messageBox("#{sNotifyMessage8}", std::vector<std::string>());
            MWBase::Environment::get().getSoundManager()->playSound("potion fail", 1.f, 1.f);
        }

        // reduce count of the ingredients
        if (mIngredient1->isUserString("ToolTipType"))
        {
            MWWorld::Ptr ingred = *mIngredient1->getUserData<MWWorld::Ptr>();
            ingred.getRefData().setCount(ingred.getRefData().getCount()-1);
            if (ingred.getRefData().getCount() == 0)
                removeIngredient(mIngredient1);
        }
        if (mIngredient2->isUserString("ToolTipType"))
        {
            MWWorld::Ptr ingred = *mIngredient2->getUserData<MWWorld::Ptr>();
            ingred.getRefData().setCount(ingred.getRefData().getCount()-1);
            if (ingred.getRefData().getCount() == 0)
                removeIngredient(mIngredient2);
        }
        if (mIngredient3->isUserString("ToolTipType"))
        {
            MWWorld::Ptr ingred = *mIngredient3->getUserData<MWWorld::Ptr>();
            ingred.getRefData().setCount(ingred.getRefData().getCount()-1);
            if (ingred.getRefData().getCount() == 0)
                removeIngredient(mIngredient3);
        }
        if (mIngredient4->isUserString("ToolTipType"))
        {
            MWWorld::Ptr ingred = *mIngredient4->getUserData<MWWorld::Ptr>();
            ingred.getRefData().setCount(ingred.getRefData().getCount()-1);
            if (ingred.getRefData().getCount() == 0)
                removeIngredient(mIngredient4);
        }
        update();
    }

    void AlchemyWindow::open()
    {
        openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        setFilter(ContainerBase::Filter_Ingredients);

        // pick the best available apparatus
        MWWorld::ContainerStore& store = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

        MWWorld::Ptr bestAlbemic;
        MWWorld::Ptr bestMortarPestle;
        MWWorld::Ptr bestCalcinator;
        MWWorld::Ptr bestRetort;

        for (MWWorld::ContainerStoreIterator it(store.begin(MWWorld::ContainerStore::Type_Apparatus));
            it != store.end(); ++it)
        {
            MWWorld::LiveCellRef<ESM::Apparatus>* ref = it->get<ESM::Apparatus>();
            if (ref->base->mData.mType == ESM::Apparatus::Albemic
            && (bestAlbemic.isEmpty() || ref->base->mData.mQuality > bestAlbemic.get<ESM::Apparatus>()->base->mData.mQuality))
                bestAlbemic = *it;
            else if (ref->base->mData.mType == ESM::Apparatus::MortarPestle
            && (bestMortarPestle.isEmpty() || ref->base->mData.mQuality > bestMortarPestle.get<ESM::Apparatus>()->base->mData.mQuality))
                bestMortarPestle = *it;
            else if (ref->base->mData.mType == ESM::Apparatus::Calcinator
            && (bestCalcinator.isEmpty() || ref->base->mData.mQuality > bestCalcinator.get<ESM::Apparatus>()->base->mData.mQuality))
                bestCalcinator = *it;
            else if (ref->base->mData.mType == ESM::Apparatus::Retort
            && (bestRetort.isEmpty() || ref->base->mData.mQuality > bestRetort.get<ESM::Apparatus>()->base->mData.mQuality))
                bestRetort = *it;
        }

        if (!bestMortarPestle.isEmpty())
        {
            mApparatus1->setUserString("ToolTipType", "ItemPtr");
            mApparatus1->setUserData(bestMortarPestle);
            mApparatus1->setImageTexture(getIconPath(bestMortarPestle));
        }
        if (!bestAlbemic.isEmpty())
        {
            mApparatus2->setUserString("ToolTipType", "ItemPtr");
            mApparatus2->setUserData(bestAlbemic);
            mApparatus2->setImageTexture(getIconPath(bestAlbemic));
        }
        if (!bestCalcinator.isEmpty())
        {
            mApparatus3->setUserString("ToolTipType", "ItemPtr");
            mApparatus3->setUserData(bestCalcinator);
            mApparatus3->setImageTexture(getIconPath(bestCalcinator));
        }
        if (!bestRetort.isEmpty())
        {
            mApparatus4->setUserString("ToolTipType", "ItemPtr");
            mApparatus4->setUserData(bestRetort);
            mApparatus4->setImageTexture(getIconPath(bestRetort));
        }
    }

    void AlchemyWindow::onIngredientSelected(MyGUI::Widget* _sender)
    {
        removeIngredient(_sender);
        drawItems();
        update();
    }

    void AlchemyWindow::onSelectedItemImpl(MWWorld::Ptr item)
    {
        MyGUI::ImageBox* add = NULL;

        // don't allow to add an ingredient that is already added
        // (which could happen if two similiar ingredients don't stack because of script / owner)
        bool alreadyAdded = false;
        std::string name = MWWorld::Class::get(item).getName(item);
        if (mIngredient1->isUserString("ToolTipType"))
        {
            MWWorld::Ptr item2 = *mIngredient1->getUserData<MWWorld::Ptr>();
            std::string name2 = MWWorld::Class::get(item2).getName(item2);
            if (name == name2)
                alreadyAdded = true;
        }
        if (mIngredient2->isUserString("ToolTipType"))
        {
            MWWorld::Ptr item2 = *mIngredient2->getUserData<MWWorld::Ptr>();
            std::string name2 = MWWorld::Class::get(item2).getName(item2);
            if (name == name2)
                alreadyAdded = true;
        }
        if (mIngredient3->isUserString("ToolTipType"))
        {
            MWWorld::Ptr item2 = *mIngredient3->getUserData<MWWorld::Ptr>();
            std::string name2 = MWWorld::Class::get(item2).getName(item2);
            if (name == name2)
                alreadyAdded = true;
        }
        if (mIngredient4->isUserString("ToolTipType"))
        {
            MWWorld::Ptr item2 = *mIngredient4->getUserData<MWWorld::Ptr>();
            std::string name2 = MWWorld::Class::get(item2).getName(item2);
            if (name == name2)
                alreadyAdded = true;
        }
        if (alreadyAdded)
            return;

        if (!mIngredient1->isUserString("ToolTipType"))
            add = mIngredient1;
        if (add == NULL  && !mIngredient2->isUserString("ToolTipType"))
            add = mIngredient2;
        if (add == NULL  && !mIngredient3->isUserString("ToolTipType"))
            add = mIngredient3;
        if (add == NULL  && !mIngredient4->isUserString("ToolTipType"))
            add = mIngredient4;

        if (add != NULL)
        {
            add->setUserString("ToolTipType", "ItemPtr");
            add->setUserData(item);
            add->setImageTexture(getIconPath(item));
            drawItems();
            update();

            std::string sound = MWWorld::Class::get(item).getUpSoundId(item);
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
        }
    }

    std::vector<MWWorld::Ptr> AlchemyWindow::itemsToIgnore()
    {
        std::vector<MWWorld::Ptr> ignore;
        // don't show ingredients that are currently selected in the "available ingredients" box.
        if (mIngredient1->isUserString("ToolTipType"))
            ignore.push_back(*mIngredient1->getUserData<MWWorld::Ptr>());
        if (mIngredient2->isUserString("ToolTipType"))
            ignore.push_back(*mIngredient2->getUserData<MWWorld::Ptr>());
        if (mIngredient3->isUserString("ToolTipType"))
            ignore.push_back(*mIngredient3->getUserData<MWWorld::Ptr>());
        if (mIngredient4->isUserString("ToolTipType"))
            ignore.push_back(*mIngredient4->getUserData<MWWorld::Ptr>());

        return ignore;
    }

    void AlchemyWindow::update()
    {
        Widgets::SpellEffectList effects;

        for (int i=0; i<4; ++i)
        {
            MyGUI::ImageBox* ingredient;
            if (i==0)
                ingredient = mIngredient1;
            else if (i==1)
                ingredient = mIngredient2;
            else if (i==2)
                ingredient = mIngredient3;
            else if (i==3)
                ingredient = mIngredient4;

            if (!ingredient->isUserString("ToolTipType"))
                continue;

            // add the effects of this ingredient to list of effects
            MWWorld::LiveCellRef<ESM::Ingredient>* ref = ingredient->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>();
            for (int i=0; i<4; ++i)
            {
                if (ref->base->mData.mEffectID[i] < 0)
                    continue;
                MWGui::Widgets::SpellEffectParams params;
                params.mEffectID = ref->base->mData.mEffectID[i];
                params.mAttribute = ref->base->mData.mAttributes[i];
                params.mSkill = ref->base->mData.mSkills[i];
                effects.push_back(params);
            }

            // update ingredient count labels
            if (ingredient->getChildCount())
                MyGUI::Gui::getInstance().destroyWidget(ingredient->getChildAt(0));

            MyGUI::TextBox* text = ingredient->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
            text->setTextAlign(MyGUI::Align::Right);
            text->setNeedMouseFocus(false);
            text->setTextShadow(true);
            text->setTextShadowColour(MyGUI::Colour(0,0,0));
            text->setCaption(getCountString(ingredient->getUserData<MWWorld::Ptr>()->getRefData().getCount()));
        }

        // now remove effects that are only present once
        Widgets::SpellEffectList::iterator it = effects.begin();
        while (it != effects.end())
        {
            Widgets::SpellEffectList::iterator next = it;
            ++next;
            bool found = false;
            for (; next != effects.end(); ++next)
            {
                if (*next == *it)
                    found = true;
            }

            if (!found)
                it = effects.erase(it);
            else
                ++it;
        }

        // now remove duplicates, and don't allow more than 4 effects
        Widgets::SpellEffectList old = effects;
        effects.clear();
        int i=0;
        for (Widgets::SpellEffectList::iterator it = old.begin();
            it != old.end(); ++it)
        {
            bool found = false;
            for (Widgets::SpellEffectList::iterator it2 = effects.begin();
                it2 != effects.end(); ++it2)
            {
                // MW considers all "foritfy attribute" effects as the same effect. See the
                // "Can't create multi-state boost potions" discussion on http://www.uesp.net/wiki/Morrowind_talk:Alchemy
                // thus, we are only checking effectID here and not attribute or skill
                if (it2->mEffectID == it->mEffectID)
                    found = true;
            }
            if (!found && i<4)
            {
                ++i;
                effects.push_back(*it);
            }
        }
        mEffects = effects;

        while (mEffectsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEffectsBox->getChildAt(0));

        MyGUI::IntCoord coord(0, 0, mEffectsBox->getWidth(), 24);
        Widgets::MWEffectListPtr effectsWidget = mEffectsBox->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, MyGUI::Align::Left | MyGUI::Align::Top);
        effectsWidget->setWindowManager(&mWindowManager);
        effectsWidget->setEffectList(effects);

        std::vector<MyGUI::WidgetPtr> effectItems;
        effectsWidget->createEffectWidgets(effectItems, mEffectsBox, coord, false, 0);
        effectsWidget->setCoord(coord);
    }

    void AlchemyWindow::removeIngredient(MyGUI::Widget* ingredient)
    {
        ingredient->clearUserStrings();
        static_cast<MyGUI::ImageBox*>(ingredient)->setImageTexture("");
        if (ingredient->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(ingredient->getChildAt(0));
    }
}
