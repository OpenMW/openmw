#include "waitdialog.hpp"

#include <MyGUI_ProgressBar.h>

#include <components/misc/rng.hpp>

#include <components/widgets/box.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwstate/charactermanager.hpp"

#include "widgets.hpp"

namespace MWGui
{

    WaitDialogProgressBar::WaitDialogProgressBar()
        : WindowBase("openmw_wait_dialog_progressbar.layout")
    {
        getWidget(mProgressBar, "ProgressBar");
        getWidget(mProgressText, "ProgressText");
    }

    void WaitDialogProgressBar::open()
    {
        center();
    }

    void WaitDialogProgressBar::setProgress (int cur, int total)
    {
        mProgressBar->setProgressRange (total);
        mProgressBar->setProgressPosition (cur);
        mProgressText->setCaption(MyGUI::utility::toString(cur) + "/" + MyGUI::utility::toString(total));
    }

    // ---------------------------------------------------------------------------------------------------------

    WaitDialog::WaitDialog()
        : WindowBase("openmw_wait_dialog.layout")
        , mTimeAdvancer(0.05f)
        , mSleeping(false)
        , mHours(1)
        , mManualHours(1)
        , mFadeTimeRemaining(0)
        , mInterruptAt(-1)
        , mProgressBar()
    {
        getWidget(mDateTimeText, "DateTimeText");
        getWidget(mRestText, "RestText");
        getWidget(mHourText, "HourText");
        getWidget(mUntilHealedButton, "UntilHealedButton");
        getWidget(mWaitButton, "WaitButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mHourSlider, "HourSlider");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &WaitDialog::onCancelButtonClicked);
        mUntilHealedButton->eventMouseButtonClick += MyGUI::newDelegate(this, &WaitDialog::onUntilHealedButtonClicked);
        mWaitButton->eventMouseButtonClick += MyGUI::newDelegate(this, &WaitDialog::onWaitButtonClicked);
        mHourSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &WaitDialog::onHourSliderChangedPosition);

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &WaitDialog::onWaitingProgressChanged);
        mTimeAdvancer.eventInterrupted += MyGUI::newDelegate(this, &WaitDialog::onWaitingInterrupted);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &WaitDialog::onWaitingFinished);

        mProgressBar.setVisible (false);
    }

    void WaitDialog::exit()
    {
        if(!mProgressBar.isVisible()) //Only exit if not currently waiting
            MWBase::Environment::get().getWindowManager()->popGuiMode();
    }

    void WaitDialog::open()
    {
        if (!MWBase::Environment::get().getWindowManager ()->getRestEnabled ())
        {
            MWBase::Environment::get().getWindowManager()->popGuiMode ();
        }

        int canRest = MWBase::Environment::get().getWorld ()->canRest ();

        if (canRest == 2)
        {
            // resting underwater or mid-air not allowed
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage1}");
            MWBase::Environment::get().getWindowManager()->popGuiMode ();
        }

        setCanRest(canRest == 0);

        onHourSliderChangedPosition(mHourSlider, 0);
        mHourSlider->setScrollPosition (0);

        std::string month = MWBase::Environment::get().getWorld ()->getMonthName();
        int hour = static_cast<int>(MWBase::Environment::get().getWorld()->getTimeStamp().getHour());
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        std::string dateTimeText =
                MyGUI::utility::toString(MWBase::Environment::get().getWorld ()->getDay ()) + " "
                + month + " (#{sDay} " + MyGUI::utility::toString(MWBase::Environment::get().getWorld ()->getTimeStamp ().getDay())
                + ") " + MyGUI::utility::toString(hour) + " " + (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mDateTimeText->setCaptionWithReplacing (dateTimeText);
    }

    void WaitDialog::onUntilHealedButtonClicked(MyGUI::Widget* sender)
    {
        int autoHours = MWBase::Environment::get().getMechanicsManager()->getHoursToRest();

        startWaiting(autoHours);
    }

    void WaitDialog::onWaitButtonClicked(MyGUI::Widget* sender)
    {
        startWaiting(mManualHours);
    }

    void WaitDialog::startWaiting(int hoursToWait)
    {
        if(Settings::Manager::getBool("autosave","Saves") && mSleeping) //autosaves when enabled and sleeping
            MWBase::Environment::get().getStateManager()->quickSave("Autosave");

        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.2f);
        mFadeTimeRemaining = 0.4f;
        setVisible(false);

        mHours = hoursToWait;

        // FIXME: move this somewhere else?
        mInterruptAt = -1;
        MWWorld::Ptr player = world->getPlayerPtr();
        if (mSleeping && player.getCell()->isExterior())
        {
            std::string regionstr = player.getCell()->getCell()->mRegion;
            if (!regionstr.empty())
            {
                const ESM::Region *region = world->getStore().get<ESM::Region>().find (regionstr);
                if (!region->mSleepList.empty())
                {
                    // figure out if player will be woken while sleeping
                    int x = Misc::Rng::rollDice(hoursToWait);
                    float fSleepRandMod = world->getStore().get<ESM::GameSetting>().find("fSleepRandMod")->getFloat();
                    if (x < fSleepRandMod * hoursToWait)
                    {
                        float fSleepRestMod = world->getStore().get<ESM::GameSetting>().find("fSleepRestMod")->getFloat();
                        int interruptAtHoursRemaining = int(fSleepRestMod * hoursToWait);
                        if (interruptAtHoursRemaining != 0)
                        {
                            mInterruptAt = hoursToWait - interruptAtHoursRemaining;
                            mInterruptCreatureList = region->mSleepList;
                        }
                    }
                }
            }
        }

        mProgressBar.setProgress (0, hoursToWait);
    }

    void WaitDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        exit();
    }

    void WaitDialog::onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position)
    {
        mHourText->setCaptionWithReplacing (MyGUI::utility::toString(position+1) + " #{sRestMenu2}");
        mManualHours = position+1;
    }

    void WaitDialog::onWaitingProgressChanged(int cur, int total)
    {
        mProgressBar.setProgress(cur, total);
        MWBase::Environment::get().getWorld()->advanceTime(1);
        MWBase::Environment::get().getMechanicsManager()->rest(mSleeping);
    }

    void WaitDialog::onWaitingInterrupted()
    {
        MWBase::Environment::get().getWindowManager()->messageBox("#{sSleepInterrupt}");
        MWBase::Environment::get().getWorld()->spawnRandomCreature(mInterruptCreatureList);
        stopWaiting();
    }

    void WaitDialog::onWaitingFinished()
    {
        stopWaiting();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWMechanics::NpcStats &pcstats = player.getClass().getNpcStats(player);

        // trigger levelup if possible
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        if (mSleeping && pcstats.getLevelProgress () >= gmst.find("iLevelUpTotal")->getInt())
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (GM_Levelup);
        }
    }

    void WaitDialog::setCanRest (bool canRest)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        bool full = (stats.getHealth().getCurrent() >= stats.getHealth().getModified())
                && (stats.getMagicka().getCurrent() >= stats.getMagicka().getModified());
        MWMechanics::NpcStats& npcstats = player.getClass().getNpcStats(player);
        bool werewolf = npcstats.isWerewolf();

        mUntilHealedButton->setVisible(canRest && !full);
        mWaitButton->setCaptionWithReplacing (canRest ? "#{sRest}" : "#{sWait}");
        mRestText->setCaptionWithReplacing (canRest ? "#{sRestMenu3}"
                                                    : (werewolf ? "#{sWerewolfRestMessage}"
                                                                : "#{sRestIllegal}"));

        mSleeping = canRest;

        Gui::Box* box = dynamic_cast<Gui::Box*>(mMainWidget);
        if (box == NULL)
            throw std::runtime_error("main widget must be a box");
        box->notifyChildrenSizeChanged();
        center();
    }

    void WaitDialog::onFrame(float dt)
    {
        mTimeAdvancer.onFrame(dt);

        if (mFadeTimeRemaining <= 0)
            return;

        mFadeTimeRemaining -= dt;

        if (mFadeTimeRemaining <= 0)
        {
            mProgressBar.setVisible(true);
            mTimeAdvancer.run(mHours, mInterruptAt);
        }
    }

    void WaitDialog::stopWaiting ()
    {
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.2f);
        mProgressBar.setVisible (false);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Rest);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_RestBed);
        mTimeAdvancer.stop();
    }


    void WaitDialog::wakeUp ()
    {
        mSleeping = false;
        mTimeAdvancer.stop();
        stopWaiting();
    }

}
