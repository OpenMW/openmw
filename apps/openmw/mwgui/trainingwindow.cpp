#include "trainingwindow.hpp"

#include <MyGUI_Gui.h>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "tooltips.hpp"

namespace
{
// Sorts a container descending by skill value. If skill value is equal, sorts ascending by skill ID.
// pair <skill ID, skill value>
bool sortSkills (const std::pair<int, int>& left, const std::pair<int, int>& right)
{
    if (left == right)
        return false;

    if (left.second > right.second)
        return true;
    else if (left.second < right.second)
        return false;

    return left.first < right.first;
}
}

namespace MWGui
{

    TrainingWindow::TrainingWindow()
        : WindowBase("openmw_trainingwindow.layout")
        , mTimeAdvancer(0.05f)
    {
        getWidget(mTrainingOptions, "TrainingOptions");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onCancelButtonClicked);

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &TrainingWindow::onTrainingProgressChanged);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &TrainingWindow::onTrainingFinished);
    }

    void TrainingWindow::onOpen()
    {
        if (mTimeAdvancer.isRunning())
        {
            mProgressBar.setVisible(true);
            setVisible(false);
        }
        else
            mProgressBar.setVisible(false);

        center();
    }

    void TrainingWindow::setPtr (const MWWorld::Ptr& actor)
    {
        mPtr = actor;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));

        // NPC can train you in his best 3 skills
        std::vector< std::pair<int, int> > skills;

        for (int i=0; i<ESM::Skill::Length; ++i)
        {
            int value = actor.getClass().getSkill(actor, i);

            skills.push_back(std::make_pair(i, value));
        }

        std::sort(skills.begin(), skills.end(), sortSkills);

        MyGUI::EnumeratorWidgetPtr widgets = mTrainingOptions->getEnumerator ();
        MyGUI::Gui::getInstance ().destroyWidgets (widgets);

        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats (player);

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        for (int i=0; i<3; ++i)
        {
            int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer
                    (mPtr,pcStats.getSkill (skills[i].first).getBase() * gmst.find("iTrainingMod")->mValue.getInteger(),true);

            MyGUI::Button* button = mTrainingOptions->createWidget<MyGUI::Button>(price <= playerGold ? "SandTextButton" : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                MyGUI::IntCoord(5, 5+i*18, mTrainingOptions->getWidth()-10, 18), MyGUI::Align::Default);

            button->setUserData(skills[i].first);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onTrainingSelected);

            button->setCaptionWithReplacing("#{" + ESM::Skill::sSkillNameIds[skills[i].first] + "} - " + MyGUI::utility::toString(price));

            button->setSize(button->getTextSize ().width+12, button->getSize().height);

            ToolTips::createSkillToolTip (button, skills[i].first);
        }

        center();
    }

    void TrainingWindow::onReferenceUnavailable ()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onCancelButtonClicked (MyGUI::Widget *sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onTrainingSelected (MyGUI::Widget *sender)
    {
        int skillId = *sender->getUserData<int>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats (player);

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        int price = pcStats.getSkill (skillId).getBase() * store.get<ESM::GameSetting>().find("iTrainingMod")->mValue.getInteger();
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,price,true);

        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        if (mPtr.getClass().getSkill(mPtr, skillId) <= pcStats.getSkill (skillId).getBase ())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sServiceTrainingWords}");
            return;
        }

        // You can not train a skill above its governing attribute
        const ESM::Skill* skill = MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(skillId);
        if (pcStats.getSkill(skillId).getBase() >= pcStats.getAttribute(skill->mData.mAttribute).getBase())
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage17}");
            return;
        }

        // increase skill
        MWWorld::LiveCellRef<ESM::NPC> *playerRef = player.get<ESM::NPC>();

        const ESM::Class *class_ =
            store.get<ESM::Class>().find(playerRef->mBase->mClass);
        pcStats.increaseSkill (skillId, *class_, true);

        // remove gold
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::NpcStats& npcStats = mPtr.getClass().getNpcStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        // advance time
        MWBase::Environment::get().getMechanicsManager()->rest(2, false);
        MWBase::Environment::get().getWorld ()->advanceTime (2);

        setVisible(false);
        mProgressBar.setVisible(true);
        mProgressBar.setProgress(0, 2);
        mTimeAdvancer.run(2);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.25);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.25, false, 0.25);
    }

    void TrainingWindow::onTrainingProgressChanged(int cur, int total)
    {
        mProgressBar.setProgress(cur, total);
    }

    void TrainingWindow::onTrainingFinished()
    {
        mProgressBar.setVisible(false);

        // go back to game mode
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Training);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    void TrainingWindow::onFrame(float dt)
    {
        checkReferenceAvailable();
        mTimeAdvancer.onFrame(dt);
    }

    bool TrainingWindow::exit()
    {
        return !mTimeAdvancer.isRunning();
    }

}
