#include "waitdialog.hpp"

#include <cmath>

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

#include "../mwmechanics/creaturestats.hpp"
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
        // we need to sleep for a specific time, and since that isn't calculated yet, we'll do it here
        // I'm making the assumption here that the # of hours rested is calculated when rest is started
        // TODO: the rougher logic here (calculating the hourly deltas) should really go into helper funcs elsewhere
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats stats = MWWorld::Class::get(player).getCreatureStats(player);
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        float hourlyHealthDelta  = stats.getAttribute(ESM::Attribute::Endurance).getModified() * 0.1;

        bool stunted = (stats.getMagicEffects().get(MWMechanics::EffectKey(ESM::MagicEffect::StuntedMagicka)).mMagnitude > 0);
        float fRestMagicMult = store.get<ESM::GameSetting>().find("fRestMagicMult")->getFloat();
        float hourlyMagickaDelta = fRestMagicMult * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

        // this massive duplication is why it has to be put into helper functions instead
        float fFatigueReturnBase = store.get<ESM::GameSetting>().find("fFatigueReturnBase")->getFloat();
        float fFatigueReturnMult = store.get<ESM::GameSetting>().find("fFatigueReturnMult")->getFloat();
        float fEndFatigueMult = store.get<ESM::GameSetting>().find("fEndFatigueMult")->getFloat();
        float capacity = MWWorld::Class::get(player).getCapacity(player);
        float encumbrance = MWWorld::Class::get(player).getEncumbrance(player);
        float normalizedEncumbrance = (capacity == 0 ? 1 : encumbrance/capacity);
        if (normalizedEncumbrance > 1)
            normalizedEncumbrance = 1;
        float hourlyFatigueDelta = fFatigueReturnBase + fFatigueReturnMult * (1 - normalizedEncumbrance);
        hourlyFatigueDelta *= 3600 * fEndFatigueMult * stats.getAttribute(ESM::Attribute::Endurance).getModified();

        float healthHours  = hourlyHealthDelta  >= 0.0
                             ? (stats.getHealth().getBase() - stats.getHealth().getCurrent()) / hourlyHealthDelta
                             : 1.0f;
        float magickaHours = stunted ? 0.0 :
                              hourlyMagickaDelta >= 0.0
                              ? (stats.getMagicka().getBase() - stats.getMagicka().getCurrent()) / hourlyMagickaDelta
                              : 1.0f;
        float fatigueHours = hourlyFatigueDelta >= 0.0
                             ? (stats.getFatigue().getBase() - stats.getFatigue().getCurrent()) / hourlyFatigueDelta
                             : 1.0f;

        int autoHours = int(std::ceil( std::max(std::max(healthHours, magickaHours), std::max(fatigueHours, 1.0f)) )); // this should use a variadic max if possible

        startWaiting(autoHours);
    }

    void WaitDialog::onWaitButtonClicked(MyGUI::Widget* sender)
    {
        startWaiting(mManualHours);
    }

    void WaitDialog::startWaiting(int hoursToWait)
    {
        MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(0.2);
        setVisible(false);
        mProgressBar.setVisible (true);

        mWaiting = true;
        mCurHour = 0;
        mHours = hoursToWait;

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
        mManualHours = position+1;
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

        while (mRemainingTime < 0)
        {
            mRemainingTime += 0.05;
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
