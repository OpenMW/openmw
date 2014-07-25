#include "levelupdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace MWGui
{
    const unsigned int LevelupDialog::sMaxCoins = 3;
    LevelupDialog::LevelupDialog()
        : WindowBase("openmw_levelup_dialog.layout"),
          mCoinCount(sMaxCoins)
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mClassImage, "ClassImage");
        getWidget(mLevelText, "LevelText");
        getWidget(mLevelDescription, "LevelDescription");
        getWidget(mCoinBox, "Coins");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onOkButtonClicked);

        for (int i=1; i<9; ++i)
        {
            MyGUI::TextBox* t;
            getWidget(t, "AttribVal" + boost::lexical_cast<std::string>(i));

            MyGUI::Button* b;
            getWidget(b, "Attrib" + boost::lexical_cast<std::string>(i));
            b->setUserData (i-1);
            b->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onAttributeClicked);
            mAttributes.push_back(b);

            mAttributeValues.push_back(t);

            getWidget(t, "AttribMultiplier" + boost::lexical_cast<std::string>(i));

            mAttributeMultipliers.push_back(t);
        }

        for (unsigned int i = 0; i < mCoinCount; ++i)
        {
            MyGUI::ImageBox* image = mCoinBox->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0,0,16,16), MyGUI::Align::Default);
            image->setImageTexture ("icons\\tx_goldicon.dds");
            mCoins.push_back(image);
        }

        center();
    }

    void LevelupDialog::setAttributeValues()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = player.getClass().getCreatureStats(player);
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats (player);

        for (int i = 0; i < 8; ++i)
        {
            int val = creatureStats.getAttribute(i).getBase();
            if (std::find(mSpentAttributes.begin(), mSpentAttributes.end(), i) != mSpentAttributes.end())
            {
                val += pcStats.getLevelupAttributeMultiplier(i);
            }

            if (val >= 100)
                val = 100;

            mAttributeValues[i]->setCaption(boost::lexical_cast<std::string>(val));
        }
    }


    void LevelupDialog::resetCoins()
    {
        const int coinSpacing = 10;
        int curX = mCoinBox->getWidth()/2 - (coinSpacing*(mCoinCount - 1) + 16*mCoinCount)/2;
        for (unsigned int i=0; i<mCoinCount; ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mCoinBox);
            image->setCoord(MyGUI::IntCoord(curX,0,16,16));
            curX += 16+coinSpacing;
        }
    }

    void LevelupDialog::assignCoins()
    {
        resetCoins();
        for (unsigned int i=0; i<mSpentAttributes.size(); ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mMainWidget);

            int attribute = mSpentAttributes[i];

            int xdiff = mAttributeMultipliers[attribute]->getCaption() == "" ? 0 : 20;

            MyGUI::IntPoint pos = mAttributes[attribute]->getAbsolutePosition() - mMainWidget->getAbsolutePosition() - MyGUI::IntPoint(22+xdiff,0);
            pos.top += (mAttributes[attribute]->getHeight() - image->getHeight())/2;
            image->setPosition(pos);
        }

        setAttributeValues();
    }

    void LevelupDialog::open()
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = player.getClass().getCreatureStats(player);
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const ESM::NPC *playerData = player.get<ESM::NPC>()->mBase;

        // set class image
        const ESM::Class *cls =
            world->getStore().get<ESM::Class>().find(playerData->mClass);

        if(world->getStore().get<ESM::Class>().isDynamic(cls->mId))
        {
            // Vanilla uses thief.dds for custom classes.
            // Choosing Stealth specialization and Speed/Agility as attributes, if possible. Otherwise fall back to first class found.
            MWWorld::SharedIterator<ESM::Class> it = world->getStore().get<ESM::Class>().begin();
            for(; it != world->getStore().get<ESM::Class>().end(); it++)
            {
                if(it->mData.mIsPlayable && it->mData.mSpecialization == 2 && it->mData.mAttribute[0] == 4 && it->mData.mAttribute[1] == 3)
                    break;
            }
            if (it == world->getStore().get<ESM::Class>().end())
                it = world->getStore().get<ESM::Class>().begin();
            if (it != world->getStore().get<ESM::Class>().end())
                cls = &*it;
        }

        mClassImage->setImageTexture ("textures\\levelup\\" + cls->mId + ".dds");

        int level = creatureStats.getLevel ()+1;
        mLevelText->setCaptionWithReplacing("#{sLevelUpMenu1} " + boost::lexical_cast<std::string>(level));

        std::string levelupdescription;
        if(level > 20)
            levelupdescription=world->getFallback()->getFallbackString("Level_Up_Default");
        else
            levelupdescription=world->getFallback()->getFallbackString("Level_Up_Level"+boost::lexical_cast<std::string>(level));

        mLevelDescription->setCaption (levelupdescription);

        unsigned int availableAttributes = 0;
        for (int i = 0; i < 8; ++i)
        {
            MyGUI::TextBox* text = mAttributeMultipliers[i];
            if (pcStats.getAttribute(i).getBase() < 100)
            {
                mAttributes[i]->setEnabled(true);
                availableAttributes++;

                int mult = pcStats.getLevelupAttributeMultiplier (i);
                text->setCaption(mult <= 1 ? "" : "x" + boost::lexical_cast<std::string>(mult));
            }
            else
            {
                mAttributes[i]->setEnabled(false);

                text->setCaption("");
            }
        }

        mCoinCount = std::min(sMaxCoins, availableAttributes);

        for (unsigned int i = 0; i < sMaxCoins; i++)
        {
            if (i < mCoinCount)
                mCoins[i]->attachToWidget(mCoinBox);
            else
                mCoins[i]->detachFromWidget();
        }

        mSpentAttributes.clear();
        resetCoins();

        setAttributeValues();

        center();
    }

    void LevelupDialog::onOkButtonClicked(MyGUI::Widget* sender)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats (player);

        if (mSpentAttributes.size() < mCoinCount)
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage36}");
        else
        {
            // increase attributes
            for (unsigned int i = 0; i < mCoinCount; ++i)
            {
                MWMechanics::AttributeValue attribute = pcStats.getAttribute(mSpentAttributes[i]);
                attribute.setBase(attribute.getBase() + pcStats.getLevelupAttributeMultiplier(mSpentAttributes[i]));

                if (attribute.getBase() >= 100)
                    attribute.setBase(100);
                pcStats.setAttribute(mSpentAttributes[i], attribute);
            }

            pcStats.levelUp();

            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Levelup);
        }

    }

    void LevelupDialog::onAttributeClicked(MyGUI::Widget *sender)
    {
        int attribute = *sender->getUserData<int>();

        std::vector<int>::iterator found = std::find(mSpentAttributes.begin(), mSpentAttributes.end(), attribute);
        if (found != mSpentAttributes.end())
            mSpentAttributes.erase(found);
        else
        {
            if (mSpentAttributes.size() == mCoinCount)
                mSpentAttributes[mCoinCount - 1] = attribute;
            else
                mSpentAttributes.push_back(attribute);
        }
        assignCoins();
    }
}
