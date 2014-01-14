#include "levelupdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace MWGui
{

    LevelupDialog::LevelupDialog()
        : WindowBase("openmw_levelup_dialog.layout")
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

        int curX = mMainWidget->getWidth()/2 - (16 + 2) * 1.5;
        for (int i=0; i<3; ++i)
        {
            MyGUI::ImageBox* image = mMainWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(curX,250,16,16), MyGUI::Align::Default);
            image->setImageTexture ("icons\\tx_goldicon.dds");
            curX += 24+2;
            mCoins.push_back(image);
        }

        center();
    }

    void LevelupDialog::setAttributeValues()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get(player).getCreatureStats (player);
        MWMechanics::NpcStats& pcStats = MWWorld::Class::get(player).getNpcStats (player);

        for (int i=0; i<8; ++i)
        {
            int val = creatureStats.getAttribute (i).getBase ();
            if (std::find(mSpentAttributes.begin(), mSpentAttributes.end(), i) != mSpentAttributes.end())
            {
                val += pcStats.getLevelupAttributeMultiplier (i);
            }

            if (val >= 100)
                val = 100;

            mAttributeValues[i]->setCaption(boost::lexical_cast<std::string>(val));
        }
    }


    void LevelupDialog::resetCoins ()
    {
        int curX = 0;
        for (int i=0; i<3; ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mCoinBox);
            image->setCoord(MyGUI::IntCoord(curX,0,16,16));
            curX += 24+2;
        }
    }

    void LevelupDialog::assignCoins ()
    {
        resetCoins();
        for (unsigned int i=0; i<mSpentAttributes.size(); ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mMainWidget);

            int attribute = mSpentAttributes[i];

            int xdiff = mAttributeMultipliers[attribute]->getCaption() == "" ? 0 : 30;

            MyGUI::IntPoint pos = mAttributes[attribute]->getAbsolutePosition() - mMainWidget->getAbsolutePosition() - MyGUI::IntPoint(24+xdiff,-4);
            image->setPosition(pos);
        }

        setAttributeValues();
    }

    void LevelupDialog::open()
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get(player).getCreatureStats (player);
        MWMechanics::NpcStats& pcStats = MWWorld::Class::get(player).getNpcStats (player);

        mSpentAttributes.clear();
        resetCoins();

        setAttributeValues();

        const ESM::NPC *playerData = player.get<ESM::NPC>()->mBase;

        // set class image
        const ESM::Class *cls =
            world->getStore().get<ESM::Class>().find(playerData->mClass);

        mClassImage->setImageTexture ("textures\\levelup\\" + cls->mId + ".dds");

        int level = creatureStats.getLevel ()+1;
        mLevelText->setCaptionWithReplacing("#{sLevelUpMenu1} " + boost::lexical_cast<std::string>(level));

        std::string levelupdescription;
        if(level>20)
            levelupdescription=world->getFallback()->getFallbackString("Level_Up_Default");
        else
            levelupdescription=world->getFallback()->getFallbackString("Level_Up_Level"+boost::lexical_cast<std::string>(level));

        mLevelDescription->setCaption (levelupdescription);

        for (int i=0; i<8; ++i)
        {
            MyGUI::TextBox* text = mAttributeMultipliers[i];
            int mult = pcStats.getLevelupAttributeMultiplier (i);
            text->setCaption(mult <= 1 ? "" : "x" + boost::lexical_cast<std::string>(mult));
        }

        center();
    }

    void LevelupDialog::onOkButtonClicked (MyGUI::Widget* sender)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get(player).getCreatureStats (player);
        MWMechanics::NpcStats& pcStats = MWWorld::Class::get(player).getNpcStats (player);

        if (mSpentAttributes.size() < 3)
            MWBase::Environment::get().getWindowManager ()->messageBox("#{sNotifyMessage36}");
        else
        {
            // increase attributes
            for (int i=0; i<3; ++i)
            {
                MWMechanics::AttributeValue attribute = creatureStats.getAttribute(mSpentAttributes[i]);
                attribute.setBase (attribute.getBase () + pcStats.getLevelupAttributeMultiplier (mSpentAttributes[i]));

                if (attribute.getBase() >= 100)
                    attribute.setBase(100);
                creatureStats.setAttribute(mSpentAttributes[i], attribute);
            }

            creatureStats.levelUp();
            pcStats.levelUp ();

            MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Levelup);
        }

    }

    void LevelupDialog::onAttributeClicked (MyGUI::Widget *sender)
    {
        int attribute = *sender->getUserData<int>();

        std::vector<int>::iterator found = std::find(mSpentAttributes.begin(), mSpentAttributes.end(), attribute);
        if (found != mSpentAttributes.end())
            mSpentAttributes.erase (found);
        else
        {
            if (mSpentAttributes.size() == 3)
                mSpentAttributes[2] = attribute;
            else
                mSpentAttributes.push_back(attribute);
        }
        assignCoins();
    }
}
