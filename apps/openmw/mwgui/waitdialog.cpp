#include "waitdialog.hpp"

#include <boost/lexical_cast.hpp>

#include <libs/openengine/ogre/fader.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/timestamp.hpp"

#include "widgets.hpp"


namespace MWGui
{

    WaitDialog::WaitDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_wait_dialog.layout", parWindowManager)
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
    }

    void WaitDialog::open()
    {
        setCanRest(MWBase::Environment::get().getWorld ()->canRest ());

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

        std::string dateTimeText =
                boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getDay ()+1) + " "
                + month + " (#{sDay} " + boost::lexical_cast<std::string>(MWBase::Environment::get().getWorld ()->getTimeStamp ().getDay ()+1)
                + ") " + boost::lexical_cast<std::string>(hour) + " " + (pm ? "#{sSaveMenuHelp05}" : "#{sSaveMenuHelp04}");

        mDateTimeText->setCaptionWithReplacing (dateTimeText);

        center();
    }

    void WaitDialog::onUntilHealedButtonClicked(MyGUI::Widget* sender)
    {

    }

    void WaitDialog::onWaitButtonClicked(MyGUI::Widget* sender)
    {
        //MWBase::Environment::get().getWorld ()->getFader ()->fadeOut(1);
    }

    void WaitDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mWindowManager.removeGuiMode (GM_Rest);
    }

    void WaitDialog::onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position)
    {
        mHourText->setCaptionWithReplacing (boost::lexical_cast<std::string>(position+1) + " #{sRestMenu2}");
    }

    void WaitDialog::setCanRest (bool canRest)
    {
        mUntilHealedButton->setVisible(canRest);
        mWaitButton->setCaptionWithReplacing (canRest ? "#{sRest}" : "#{sWait}");
        mRestText->setCaptionWithReplacing (canRest ? "#{sRestMenu3}" : "#{sRestIllegal}");
    }

}
