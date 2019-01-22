#include "levelupdialog.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_EditBox.h>

#include <components/fallback/fallback.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "class.hpp"

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
        getWidget(mAssignWidget, "AssignWidget");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onOkButtonClicked);

        for (int i=1; i<9; ++i)
        {
            MyGUI::TextBox* t;
            getWidget(t, "AttribVal" + MyGUI::utility::toString(i));
            mAttributeValues.push_back(t);

            MyGUI::Button* b;
            getWidget(b, "Attrib" + MyGUI::utility::toString(i));
            b->setUserData (i-1);
            b->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onAttributeClicked);
            mAttributes.push_back(b);

            getWidget(t, "AttribMultiplier" + MyGUI::utility::toString(i));
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

            mAttributeValues[i]->setCaption(MyGUI::utility::toString(val));
        }
    }


    void LevelupDialog::resetCoins()
    {
        const int coinSpacing = 33;
        int curX = mCoinBox->getWidth()/2 - (coinSpacing*(mCoinCount - 1) + 16*mCoinCount)/2;
        for (unsigned int i=0; i<sMaxCoins; ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mCoinBox);
            if (i < mCoinCount)
            {
                mCoins[i]->setVisible(true);
                image->setCoord(MyGUI::IntCoord(curX,0,16,16));
                curX += 16+coinSpacing;
            }
            else
                mCoins[i]->setVisible(false);
        }
    }

    void LevelupDialog::assignCoins()
    {
        resetCoins();
        for (unsigned int i=0; i<mSpentAttributes.size(); ++i)
        {
            MyGUI::ImageBox* image = mCoins[i];
            image->detachFromWidget();
            image->attachToWidget(mAssignWidget);

            int attribute = mSpentAttributes[i];

            int xdiff = mAttributeMultipliers[attribute]->getCaption() == "" ? 0 : 20;

            MyGUI::IntPoint pos = mAttributes[attribute]->getAbsolutePosition() - mAssignWidget->getAbsolutePosition() - MyGUI::IntPoint(22+xdiff,0);
            pos.top += (mAttributes[attribute]->getHeight() - image->getHeight())/2;
            image->setPosition(pos);
        }

        setAttributeValues();
    }

    void LevelupDialog::onOpen()
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        MWMechanics::CreatureStats& creatureStats = player.getClass().getCreatureStats(player);
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        setClassImage(mClassImage, getLevelupClassImage(pcStats.getSkillIncreasesForSpecialization(0),
                                                        pcStats.getSkillIncreasesForSpecialization(1),
                                                        pcStats.getSkillIncreasesForSpecialization(2)));

        int level = creatureStats.getLevel ()+1;
        mLevelText->setCaptionWithReplacing("#{sLevelUpMenu1} " + MyGUI::utility::toString(level));

        std::string levelupdescription;
        levelupdescription = Fallback::Map::getString("Level_Up_Level"+MyGUI::utility::toString(level));

        if (levelupdescription == "")
            levelupdescription = Fallback::Map::getString("Level_Up_Default");

        mLevelDescription->setCaption (levelupdescription);

        unsigned int availableAttributes = 0;
        for (int i = 0; i < 8; ++i)
        {
            MyGUI::TextBox* text = mAttributeMultipliers[i];
            if (pcStats.getAttribute(i).getBase() < 100)
            {
                mAttributes[i]->setEnabled(true);
                mAttributeValues[i]->setEnabled(true);
                availableAttributes++;

                int mult = pcStats.getLevelupAttributeMultiplier (i);
                mult = std::min(mult, 100-pcStats.getAttribute(i).getBase());
                text->setCaption(mult <= 1 ? "" : "x" + MyGUI::utility::toString(mult));
            }
            else
            {
                mAttributes[i]->setEnabled(false);
                mAttributeValues[i]->setEnabled(false);

                text->setCaption("");
            }
        }

        mCoinCount = std::min(sMaxCoins, availableAttributes);

        mSpentAttributes.clear();
        resetCoins();

        setAttributeValues();

        center();

        // Play LevelUp Music
        MWBase::Environment::get().getSoundManager()->streamMusic("Special/MW_Triumph.mp3");
    }

    void LevelupDialog::onOkButtonClicked(MyGUI::Widget* sender)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
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

    std::string LevelupDialog::getLevelupClassImage(const int combatIncreases, const int magicIncreases, const int stealthIncreases)
    {
        std::string ret = "acrobat";

        int total = combatIncreases + magicIncreases + stealthIncreases;
        if (total == 0)
            return ret;

        int combatFraction = static_cast<int>(static_cast<float>(combatIncreases) / total * 10.f);
        int magicFraction = static_cast<int>(static_cast<float>(magicIncreases) / total * 10.f);
        int stealthFraction = static_cast<int>(static_cast<float>(stealthIncreases) / total * 10.f);

        if (combatFraction > 7)
            ret = "warrior";
        else if (magicFraction > 7)
            ret = "mage";
        else if (stealthFraction > 7)
            ret = "thief";

        switch (combatFraction)
        {
            case 7:
                ret = "warrior";
                break;
            case 6:
                if (stealthFraction == 1)
                    ret = "barbarian";
                else if (stealthFraction == 3)
                    ret = "crusader";
                else
                    ret = "knight";
                break;
            case 5:
                if (stealthFraction == 3)
                    ret = "scout";
                else
                    ret = "archer";
                break;
            case 4:
                ret = "rogue";
                break;
            default:
                break;
        }

        switch (magicFraction)
        {
            case 7:
                ret = "mage";
                break;
            case 6:
                if (combatFraction == 2)
                    ret = "sorcerer";
                else if (combatIncreases == 3)
                    ret = "healer";
                else
                    ret = "battlemage";
                break;
            case 5:
                ret = "witchhunter";
                break;
            case 4:
                ret = "spellsword";
                // In vanilla there's also code for "nightblade", however it seems to be unreachable.
                break;
            default:
                break;
        }

        switch (stealthFraction)
        {
            case 7:
                ret = "thief";
                break;
            case 6:
                if (magicFraction == 1)
                    ret = "agent";
                else if (magicIncreases == 3)
                    ret = "assassin";
                else
                    ret = "acrobat";
                break;
            case 5:
                if (magicIncreases == 3)
                    ret = "monk";
                else
                    ret = "pilgrim";
                break;
            case 3:
                if (magicFraction == 3)
                    ret = "bard";
                break;
            default:
                break;
        }

        return ret;
    }
}
