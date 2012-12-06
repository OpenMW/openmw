#include "waitdialog.hpp"

#include <boost/lexical_cast.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/timestamp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "widgets.hpp"


namespace MWGui
{

    WaitDialogProgressBar::WaitDialogProgressBar(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_wait_dialog_progressbar.layout", parWindowManager)
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

    WaitDialog::WaitDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_wait_dialog.layout", parWindowManager)
        , mProgressBar(parWindowManager)
        , mWaiting(false)
        , mSleeping(false)
        , mHours(1)
        , mRemainingTime(0.05)
    {
        getWidget(mDateTimeText, "DateTimeText");
        getWidget(mRestText, "RestText");
        getWidget(mHourText, "HourText");
        getWidget(mHourSlider, "HourSlider");
        getWidget(mUntilHealedButton, "UntilHealedButton");
        getWidget(mWaitButton, "WaitButton");
        getWidget(mCancelButton, "CancelButton");

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
            mWindowManager.popGuiMode ();
        }

        int canRest = MWBase::Environment::get().getWorld ()->canRest ();

        if (canRest == 2)
        {
            // resting underwater or mid-air not allowed
            mWindowManager.messageBox ("#{sNotifyMessage1}", std::vector<std::string>());
            mWindowManager.popGuiMode ();
        }

        setCanRest(canRest == 0);

        onHourSliderChangedPosition(mHourSlider, 0);
        mHourSlider->setScrollPosition (0);

        // http://www.uesp.net/wiki/Lore:Calendar
        std::string month;
        int m = MWBase::Environment::get().getWorld ()->getMonth ();
        if (m == 0)
            month = "#{sMonthMorningstar}";
        else if (m == 1)
            month = "#{sMonthSunsdawn}";
        else if (m == 2)
            month = "#{sMonthFirstseed}";
        else if (m == 3)
            month = "#{sMonthRainshand}";
        else if (m == 4)
            month = "#{sMonthSecondseed}";
        else if (m == 5)
            month = "#{sMonthMidyear}";
        else if (m == 6)
            month = "#{sMonthSunsheight}";
        else if (m == 7)
            month = "#{sMonthLastseed}";
        else if (m == 8)
            month = "#{sMonthHeartfire}";
        else if (m == 9)
            month = "#{sMonthFrostfall}";
        else if (m == 10)
            month = "#{sMonthSunsdusk}";
        else if (m == 11)
            month = "#{sMonthEveningstar}";

        int hour = MWBase::Environment::get().getWorld ()->getTimeStamp ().getHour ();
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;

        std::string dateTimeText =
                boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getDay ()) + " "
                + month + " (#{sDay} " + boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getTimeStamp ().getDay ()+1)
                + ") " + boost::lexical_cast<std::string>(hour) + " " + (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mDateTimeText->setCaptionWithReplacing (dateTimeText);
    }

    void WaitDialog::onUntilHealedButtonClicked(MyGUI::Widget* sender)
    {
        startWaiting();
    }

    void WaitDialog::onWaitButtonClicked(MyGUI::Widget* sender)
    {
        startWaiting();
    }

    void WaitDialog::startWaiting ()
    {
        MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(0.2);
        setVisible(false);
        mProgressBar.setVisible (true);
        mWaiting = true;
        mCurHour = 0;
        mRemainingTime = 0.05;
        mProgressBar.setProgress (0, mHours);
    }

    void WaitDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mWindowManager.popGuiMode ();
    }

    void WaitDialog::onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position)
    {
        mHourText->setCaptionWithReplacing (boost::lexical_cast<std::string>(position+1) + " #{sRestMenu2}");
        mHours = position+1;
    }

    void WaitDialog::setCanRest (bool canRest)
    {
        mUntilHealedButton->setVisible(canRest);
        mWaitButton->setCaptionWithReplacing (canRest ? "#{sRest}" : "#{sWait}");
        mRestText->setCaptionWithReplacing (canRest ? "#{sRestMenu3}" : "#{sRestIllegal}");

        mSleeping = canRest;

        dynamic_cast<Widgets::Box*>(mMainWidget)->notifyChildrenSizeChanged();
        center();
    }

    void WaitDialog::onFrame(float dt)
    {
        if (!mWaiting)
            return;

        mRemainingTime -= dt;

        if (mRemainingTime < 0)
        {
            mRemainingTime = 0.05;
            ++mCurHour;
            mProgressBar.setProgress (mCurHour, mHours);

            if (mCurHour <= mHours)
            {
                MWBase::Environment::get().getWorld ()->advanceTime (1);
                if (mSleeping)
                    MWBase::Environment::get().getMechanicsManager ()->restoreDynamicStats ();
            }
        }

        if (mCurHour > mHours)
            stopWaiting();
    }

    void WaitDialog::stopWaiting ()
    {
        MWBase::Environment::get().getWorld ()->getFader ()->fadeIn(0.2);
        mProgressBar.setVisible (false);
        mWindowManager.removeGuiMode (GM_Rest);
        mWindowManager.removeGuiMode (GM_RestBed);
        mWaiting = false;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::NpcStats pcstats = MWWorld::Class::get(player).getNpcStats(player);

        // trigger levelup if possible
        if (mSleeping && pcstats.getLevelProgress () >= 10)
        {
            mWindowManager.pushGuiMode (GM_Levelup);
        }
    }

    void WaitDialog::wakeUp ()
    {
        mSleeping = false;
        mWaiting = false;
        stopWaiting();
    }

}
