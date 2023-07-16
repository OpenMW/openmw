#include "trainingwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextIterator.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/settings/values.hpp>

#include "tooltips.hpp"

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

    void TrainingWindow::setPtr(const MWWorld::Ptr& actor)
    {
        mPtr = actor;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));

        const auto& store = MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::GameSetting>& gmst = store->get<ESM::GameSetting>();
        const MWWorld::Store<ESM::Skill>& skillStore = store->get<ESM::Skill>();

        // NPC can train you in his best 3 skills
        std::vector<std::pair<const ESM::Skill*, float>> skills;

        MWMechanics::NpcStats const& actorStats(actor.getClass().getNpcStats(actor));
        for (const ESM::Skill& skill : skillStore)
        {
            float value = getSkillForTraining(actorStats, skill.mId);

            skills.emplace_back(&skill, value);
        }

        std::sort(skills.begin(), skills.end(), [](const auto& left, const auto& right) {
            return std::tie(right.second, left.first->mId) < std::tie(left.second, right.first->mId);
        });

        MyGUI::EnumeratorWidgetPtr widgets = mTrainingOptions->getEnumerator();
        MyGUI::Gui::getInstance().destroyWidgets(widgets);

        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const int lineHeight = Settings::gui().mFontSize + 2;

        for (int i = 0; i < 3; ++i)
        {
            const ESM::Skill* skill = skills[i].first;
            int price = static_cast<int>(
                pcStats.getSkill(skill->mId).getBase() * gmst.find("iTrainingMod")->mValue.getInteger());
            price = std::max(1, price);
            price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

            MyGUI::Button* button = mTrainingOptions->createWidget<MyGUI::Button>(price <= playerGold
                    ? "SandTextButton"
                    : "SandTextButtonDisabled", // can't use setEnabled since that removes tooltip
                MyGUI::IntCoord(5, 5 + i * lineHeight, mTrainingOptions->getWidth() - 10, lineHeight),
                MyGUI::Align::Default);

            button->setUserData(skills[i].first);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onTrainingSelected);

            button->setCaptionWithReplacing(
                MyGUI::TextIterator::toTagsString(skill->mName) + " - " + MyGUI::utility::toString(price));

            button->setSize(button->getTextSize().width + 12, button->getSize().height);

            ToolTips::createSkillToolTip(button, skill->mId);
        }

        center();
    }

    void TrainingWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
    }

    void TrainingWindow::onTrainingSelected(MyGUI::Widget* sender)
    {
        const ESM::Skill* skill = *sender->getUserData<const ESM::Skill*>();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        int price = pcStats.getSkill(skill->mId).getBase()
            * store.get<ESM::GameSetting>().find("iTrainingMod")->mValue.getInteger();
        price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, price, true);

        if (price > player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId))
            return;

        if (getSkillForTraining(mPtr.getClass().getNpcStats(mPtr), skill->mId)
            <= pcStats.getSkill(skill->mId).getBase())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sServiceTrainingWords}");
            return;
        }

        // You can not train a skill above its governing attribute
        if (pcStats.getSkill(skill->mId).getBase()
            >= pcStats.getAttribute(ESM::Attribute::AttributeID(skill->mData.mAttribute)).getBase())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage17}");
            return;
        }

        // increase skill
        MWWorld::LiveCellRef<ESM::NPC>* playerRef = player.get<ESM::NPC>();

        const ESM::Class* class_ = store.get<ESM::Class>().find(playerRef->mBase->mClass);
        pcStats.increaseSkill(skill->mId, *class_, true);

        // remove gold
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::NpcStats& npcStats = mPtr.getClass().getNpcStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        setVisible(false);
        mProgressBar.setVisible(true);
        mProgressBar.setProgress(0, 2);
        mTimeAdvancer.run(2);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.2);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.2, false, 0.2);
    }

    void TrainingWindow::onTrainingProgressChanged(int cur, int total)
    {
        mProgressBar.setProgress(cur, total);
    }

    void TrainingWindow::onTrainingFinished()
    {
        mProgressBar.setVisible(false);

        // advance time
        MWBase::Environment::get().getMechanicsManager()->rest(2, false);
        MWBase::Environment::get().getWorld()->advanceTime(2);

        // go back to game mode
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Training);
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
    }

    float TrainingWindow::getSkillForTraining(const MWMechanics::NpcStats& stats, ESM::RefId id) const
    {
        if (Settings::game().mTrainersTrainingSkillsBasedOnBaseSkill)
            return stats.getSkill(id).getBase();
        return stats.getSkill(id).getModified();
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
