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
        // check if mortar & pestle is available (always needed)
        /// \todo check albemic, calcinator, retort (sometimes needed)
        if (!mApparatus[0]->isUserString("ToolTipType"))
        {
            mWindowManager.messageBox("#{sNotifyMessage45}", std::vector<std::string>());
            return;
        }

        // make sure 2 or more ingredients were selected
        int numIngreds = 0;
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
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
            for (int i=0; i<4; ++i)
                if (mIngredients[i]->isUserString("ToolTipType"))
                    weight += mIngredients[i]->getUserData<MWWorld::Ptr>()->get<ESM::Ingredient>()->base->mData.mWeight;
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
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
            {
                MWWorld::Ptr ingred = *mIngredients[i]->getUserData<MWWorld::Ptr>();
                ingred.getRefData().setCount(ingred.getRefData().getCount()-1);
                if (ingred.getRefData().getCount() == 0)
                    removeIngredient(mIngredients[i]);
        }

        update();
    }

    void AlchemyWindow::open()
    {
        openContainer (MWBase::Environment::get().getWorld()->getPlayer().getPlayer()); // this sets mPtr
        setFilter (ContainerBase::Filter_Ingredients);
        
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
        for (int i=0; i<4; ++i)       
            if (mIngredients[i]->isUserString("ToolTipType"))
            {
                MWWorld::Ptr item2 = *mIngredients[i]->getUserData<MWWorld::Ptr>();
                std::string name2 = MWWorld::Class::get(item2).getName(item2);
                if (name == name2)
                    alreadyAdded = true;
            }

        if (alreadyAdded)
            return;

        for (int i=0; i<4; ++i)
            if (!mIngredients[i]->isUserString("ToolTipType"))
            {
                add = mIngredients[i];
                break;
            }

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
        for (int i=0; i<4; ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
                ignore.push_back(*mIngredients[i]->getUserData<MWWorld::Ptr>());

        return ignore;
    }

    void AlchemyWindow::update()
    {
        Widgets::SpellEffectList effects;

        for (int i=0; i<4; ++i)
        {
            MyGUI::ImageBox* ingredient = mIngredients[i];

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
