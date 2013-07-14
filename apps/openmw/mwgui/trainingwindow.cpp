#include "trainingwindow.hpp"

#include <boost/lexical_cast.hpp>

#include <openengine/ogre/fader.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "inventorywindow.hpp"
#include "tradewindow.hpp"
#include "tooltips.hpp"

namespace MWGui
{

    TrainingWindow::TrainingWindow()
        : WindowBase("openmw_trainingwindow.layout")
        , mFadeTimeRemaining(0)
    {
        getWidget(mTrainingOptions, "TrainingOptions");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onCancelButtonClicked);
    }

    void TrainingWindow::open()
    {
        center();
    }

    void TrainingWindow::startTraining (MWWorld::Ptr actor)
    {
        mPtr = actor;

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + boost::lexical_cast<std::string>(MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold()));

        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(actor).getNpcStats (actor);

        // NPC can train you in his best 3 skills
        std::vector< std::pair<int, int> > bestSkills;
        bestSkills.push_back (std::make_pair(-1, -1));
        bestSkills.push_back (std::make_pair(-1, -1));
        bestSkills.push_back (std::make_pair(-1, -1));

        for (int i=0; i<ESM::Skill::Length; ++i)
        {
            int value = npcStats.getSkill (i).getBase ();

            for (int j=0; j<3; ++j)
            {
                if (value > bestSkills[j].second)
                {
                    if (j<2)
                    {
                        bestSkills[j+1] = bestSkills[j];
                    }
                    bestSkills[j] = std::make_pair(i, value);
                    break;
                }
            }
        }

        MyGUI::EnumeratorWidgetPtr widgets = mTrainingOptions->getEnumerator ();
        MyGUI::Gui::getInstance ().destroyWidgets (widgets);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();
        MWMechanics::NpcStats& pcStats = MWWorld::Class::get(player).getNpcStats (player);

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        for (int i=0; i<3; ++i)
        {
            int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer
                    (mPtr,pcStats.getSkill (bestSkills[i].first).getBase() * gmst.find("iTrainingMod")->getInt (),true);

            std::string skin = (price > MWBase::Environment::get().getWindowManager()->getInventoryWindow ()->getPlayerGold ()) ? "SandTextGreyedOut" : "SandTextButton";

            MyGUI::Button* button = mTrainingOptions->createWidget<MyGUI::Button>(skin,
                MyGUI::IntCoord(5, 5+i*18, mTrainingOptions->getWidth()-10, 18), MyGUI::Align::Default);

            button->setUserData(bestSkills[i].first);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onTrainingSelected);

            button->setCaptionWithReplacing("#{" + ESM::Skill::sSkillNameIds[bestSkills[i].first] + "} - " + boost::lexical_cast<std::string>(price));

            button->setSize(button->getTextSize ().width+12, button->getSize().height);

            ToolTips::createSkillToolTip (button, bestSkills[i].first);
        }

        center();
    }

    void TrainingWindow::onReferenceUnavailable ()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onCancelButtonClicked (MyGUI::Widget *sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Training);
    }

    void TrainingWindow::onTrainingSelected (MyGUI::Widget *sender)
    {
        int skillId = *sender->getUserData<int>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();
        MWMechanics::NpcStats& pcStats = MWWorld::Class::get(player).getNpcStats (player);

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        int price = pcStats.getSkill (skillId).getBase() * store.get<ESM::GameSetting>().find("iTrainingMod")->getInt ();
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,price,true);

        if (MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold()<price)
            return;

        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(mPtr).getNpcStats (mPtr);
        if (npcStats.getSkill (skillId).getBase () <= pcStats.getSkill (skillId).getBase ())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sServiceTrainingWords}");
            return;
        }

        // increase skill
        MWWorld::LiveCellRef<ESM::NPC> *playerRef = player.get<ESM::NPC>();

        const ESM::Class *class_ =
            store.get<ESM::Class>().find(playerRef->mBase->mClass);
        pcStats.increaseSkill (skillId, *class_, true);

        // remove gold
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->addOrRemoveGold(-price);

        // go back to game mode
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Training);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Dialogue);

        // advance time
        MWBase::Environment::get().getWorld ()->advanceTime (2);

        MWBase::Environment::get().getWorld ()->getFader()->fadeOut(0.25);
        mFadeTimeRemaining = 0.5;
    }

    void TrainingWindow::onFrame(float dt)
    {
        if (mFadeTimeRemaining <= 0)
            return;

        mFadeTimeRemaining -= dt;

        if (mFadeTimeRemaining <= 0)
            MWBase::Environment::get().getWorld ()->getFader()->fadeIn(0.25);
    }
}
