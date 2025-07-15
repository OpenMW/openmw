#include "trainingwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextIterator.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
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
    {
        getWidget(mTrainingOptions, "TrainingOptions");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mPlayerGold, "PlayerGold");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TrainingWindow::onCancelButtonClicked);

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &TrainingWindow::onTrainingProgressChanged);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &TrainingWindow::onTrainingFinished);

        if (Settings::gui().mControllerMenus)
        {
            mDisableGamepadCursor = true;
            mControllerButtons.a = "#{sBuy}";
            mControllerButtons.b = "#{Interface:Cancel}";
        }
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
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in TrainingWindow::setPtr");
        mPtr = actor;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));

        const auto& store = MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::GameSetting>& gmst = store->get<ESM::GameSetting>();
        const MWWorld::Store<ESM::Skill>& skillStore = store->get<ESM::Skill>();

        // NPC can train you in their best 3 skills
        constexpr size_t maxSkills = 3;
        std::vector<std::pair<const ESM::Skill*, float>> skills;
        skills.reserve(maxSkills);

        const auto sortByValue
            = [](const std::pair<const ESM::Skill*, float>& lhs, const std::pair<const ESM::Skill*, float>& rhs) {
                  return lhs.second > rhs.second;
              };
        // Maintain a sorted vector of max maxSkills elements, ordering skills by value and content file order
        const MWMechanics::NpcStats& actorStats = actor.getClass().getNpcStats(actor);
        for (const ESM::Skill& skill : skillStore)
        {
            float value = getSkillForTraining(actorStats, skill.mId);
            if (skills.size() < maxSkills)
            {
                skills.emplace_back(&skill, value);
                std::stable_sort(skills.begin(), skills.end(), sortByValue);
            }
            else
            {
                auto& lowest = skills[maxSkills - 1];
                if (lowest.second < value)
                {
                    lowest.first = &skill;
                    lowest.second = value;
                    std::stable_sort(skills.begin(), skills.end(), sortByValue);
                }
            }
        }

        MyGUI::EnumeratorWidgetPtr widgets = mTrainingOptions->getEnumerator();
        MyGUI::Gui::getInstance().destroyWidgets(widgets);

        MWMechanics::NpcStats& pcStats = player.getClass().getNpcStats(player);

        const int lineHeight = Settings::gui().mFontSize + 2;

        mTrainingButtons.clear();
        for (size_t i = 0; i < skills.size(); ++i)
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

            if (price <= playerGold)
                mTrainingButtons.emplace_back(button);
        }

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = 0;
            if (mTrainingButtons.size() > 0)
                mTrainingButtons[0]->setStateSelected(true);
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
            >= pcStats.getAttribute(ESM::Attribute::indexToRefId(skill->mData.mAttribute)).getModified())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage17}");
            return;
        }

        // increase skill
        MWBase::Environment::get().getLuaManager()->skillLevelUp(player, skill->mId, "trainer");

        // remove gold
        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price);

        // add gold to NPC trading gold pool
        MWMechanics::NpcStats& npcStats = mPtr.getClass().getNpcStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        setVisible(false);
        mProgressBar.setVisible(true);
        mProgressBar.setProgress(0, 2);
        mTimeAdvancer.run(2);

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.2f);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.2f, false, 0.2f);
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

    bool TrainingWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus < mTrainingButtons.size())
                onTrainingSelected(mTrainingButtons[mControllerFocus]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelButtonClicked(mCancelButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            if (mTrainingButtons.size() <= 1)
                return true;

            setControllerFocus(mTrainingButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus - 1, mTrainingButtons.size());
            setControllerFocus(mTrainingButtons, mControllerFocus, true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            if (mTrainingButtons.size() <= 1)
                return true;

            setControllerFocus(mTrainingButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus + 1, mTrainingButtons.size());
            setControllerFocus(mTrainingButtons, mControllerFocus, true);
        }

        return true;
    }
}
