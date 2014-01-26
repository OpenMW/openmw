#include "waitdialog.hpp"

#include <boost/lexical_cast.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"


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

        // http://www.uesp.net/wiki/Lore:Calendar
        std::string month;
        int m = MWBase::Environment::get().getWorld ()->getMonth ();
        switch (m) {
            case 0:
                month = "#{sMonthMorningstar}";
                break;
            case 1:
                month = "#{sMonthSunsdawn}";
                break;
            case 2:
                month = "#{sMonthFirstseed}";
                break;
            case 3:
                month = "#{sMonthRainshand}";
                break;
            case 4:
                month = "#{sMonthSecondseed}";
                break;
            case 5:
                month = "#{sMonthMidyear}";
                break;
            case 6:
                month = "#{sMonthSunsheight}";
                break;
            case 7:
                month = "#{sMonthLastseed}";
                break;
            case 8:
                month = "#{sMonthHeartfire}";
                break;
            case 9:
                month = "#{sMonthFrostfall}";
                break;
            case 10:
                month = "#{sMonthSunsdusk}";
                break;
            case 11:
                month = "#{sMonthEveningstar}";
                break;
            default:
                break;
        }
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
            std::string regionstr = player.getCell()->mCell->mRegion;
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
        MWBase::Environment::get().getWindowManager()->popGuiMode ();
    }

    void WaitDialog::onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position)
    {
        mHourText->setCaptionWithReplacing (boost::lexical_cast<std::string>(position+1) + " #{sRestMenu2}");
        mManualHours = position+1;
    }

    void WaitDialog::setCanRest (bool canRest)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        bool full = (stats.getFatigue().getCurrent() >= stats.getFatigue().getModified())
                && (stats.getHealth().getCurrent() >= stats.getHealth().getModified())
                && (stats.getMagicka().getCurrent() >= stats.getMagicka().getModified());
        MWMechanics::NpcStats& npcstats = MWWorld::Class::get(player).getNpcStats(player);
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
            stopWaiting();

    }

    void WaitDialog::stopWaiting ()
    {
        MWBase::Environment::get().getWorld ()->getFader ()->fadeIn(0.2);
        mProgressBar.setVisible (false);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Rest);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_RestBed);
        mWaiting = false;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::NpcStats &pcstats = MWWorld::Class::get(player).getNpcStats(player);

        // trigger levelup if possible
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        if (mSleeping && pcstats.getLevelProgress () >= gmst.find("iLevelUpTotal")->getInt())
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (GM_Levelup);
        }
    }

    void WaitDialog::wakeUp ()
    {
        mSleeping = false;
        mWaiting = false;
        stopWaiting();
    }

}
