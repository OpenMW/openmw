#include "waitdialog.hpp"

#include <boost/lexical_cast.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwstate/charactermanager.hpp"

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
        mProgressText->setCaption(boost::lexical_cast<std::string>(cur) + "/" + boost::lexical_cast<std::string>(total));
    }

    // ---------------------------------------------------------------------------------------------------------

    WaitDialog::WaitDialog()
        : WindowBase("openmw_wait_dialog.layout")
        , mProgressBar()
        , mWaiting(false)
        , mSleeping(false)
        , mHours(1)
        , mRemainingTime(0.05)
        , mCurHour(0)
        , mManualHours(1)
        , mInterruptAt(-1)
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
        int hour = MWBase::Environment::get().getWorld ()->getTimeStamp ().getHour ();
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        std::string dateTimeText =
                boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getDay ()) + " "
                + month + " (#{sDay} " + boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getTimeStamp ().getDay())
                + ") " + boost::lexical_cast<std::string>(hour) + " " + (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

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
        world->getFader ()->fadeOut(0.2);
        setVisible(false);
        mProgressBar.setVisible (true);

        mWaiting = true;
        mCurHour = 0;
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
                    float fSleepRandMod = world->getStore().get<ESM::GameSetting>().find("fSleepRandMod")->getFloat();
                    int x = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * hoursToWait; // [0, hoursRested]
                    float y = fSleepRandMod * hoursToWait;
                    if (x > y)
                    {
                        float fSleepRestMod = world->getStore().get<ESM::GameSetting>().find("fSleepRestMod")->getFloat();
                        mInterruptAt = hoursToWait - int(fSleepRestMod * hoursToWait);
                        mInterruptCreatureList = region->mSleepList;
                    }
                }
            }
        }

        mRemainingTime = 0.05;
        mProgressBar.setProgress (0, mHours);
    }

    void WaitDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        exit();
    }

    void WaitDialog::onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position)
    {
        mHourText->setCaptionWithReplacing (boost::lexical_cast<std::string>(position+1) + " #{sRestMenu2}");
        mManualHours = position+1;
    }

    void WaitDialog::setCanRest (bool canRest)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        bool full = (stats.getFatigue().getCurrent() >= stats.getFatigue().getModified())
                && (stats.getHealth().getCurrent() >= stats.getHealth().getModified())
                && (stats.getMagicka().getCurrent() >= stats.getMagicka().getModified());
        MWMechanics::NpcStats& npcstats = player.getClass().getNpcStats(player);
        bool werewolf = npcstats.isWerewolf();

        mUntilHealedButton->setVisible(canRest && !full);
        mWaitButton->setCaptionWithReplacing (canRest ? "#{sRest}" : "#{sWait}");
        mRestText->setCaptionWithReplacing (canRest ? "#{sRestMenu3}"
                                                    : (werewolf ? "#{sWerewolfRestMessage}"
                                                                : "#{sRestIllegal}"));

        mSleeping = canRest;

        dynamic_cast<Widgets::Box*>(mMainWidget)->notifyChildrenSizeChanged();
        center();
    }

    void WaitDialog::onFrame(float dt)
    {
        if (!mWaiting)
            return;

        if (mCurHour == mInterruptAt)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sSleepInterrupt}");
            MWBase::Environment::get().getWorld()->spawnRandomCreature(mInterruptCreatureList);
            stopWaiting();
        }

        mRemainingTime -= dt;

        while (mRemainingTime < 0)
        {
            mRemainingTime += 0.05;
            ++mCurHour;
            mProgressBar.setProgress (mCurHour, mHours);

            if (mCurHour <= mHours)
            {
                MWBase::Environment::get().getWorld ()->advanceTime (1);
                MWBase::Environment::get().getMechanicsManager ()->rest (mSleeping);
            }
        }

        if (mCurHour > mHours)
        {
            stopWaiting();

            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            const MWMechanics::NpcStats &pcstats = player.getClass().getNpcStats(player);

            // trigger levelup if possible
            const MWWorld::Store<ESM::GameSetting> &gmst =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
            if (mSleeping && pcstats.getLevelProgress () >= gmst.find("iLevelUpTotal")->getInt())
            {
                MWBase::Environment::get().getWindowManager()->pushGuiMode (GM_Levelup);
            }
        }
    }

    void WaitDialog::stopWaiting ()
    {
        MWBase::Environment::get().getWorld ()->getFader ()->fadeIn(0.2);
        mProgressBar.setVisible (false);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Rest);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_RestBed);
        mWaiting = false;
    }


    void WaitDialog::wakeUp ()
    {
        mSleeping = false;
        mWaiting = false;
        stopWaiting();
    }

}
