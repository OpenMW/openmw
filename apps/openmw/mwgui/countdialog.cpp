#include "countdialog.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_RenderManager.h>

#include <components/widgets/numericeditbox.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    CountDialog::CountDialog() :
        WindowModal("openmw_count_window.layout")
    {
        getWidget(mSlider, "CountSlider");
        getWidget(mItemEdit, "ItemEdit");
        getWidget(mItemText, "ItemText");
        getWidget(mLabelText, "LabelText");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CountDialog::onCancelButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CountDialog::onOkButtonClicked);
        mItemEdit->eventValueChanged += MyGUI::newDelegate(this, &CountDialog::onEditValueChanged);
        mSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &CountDialog::onSliderMoved);
        // make sure we read the enter key being pressed to accept multiple items
        mItemEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &CountDialog::onEnterKeyPressed);
    }

    void CountDialog::openCountDialog(const std::string& item, const std::string& message, const int maxCount)
    {
        setVisible(true);

        mLabelText->setCaptionWithReplacing(message);

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        mSlider->setScrollRange(maxCount);
        mItemText->setCaption(item);

        int width = std::max(mItemText->getTextSize().width + 128, 320);
        setCoord(viewSize.width/2 - width/2,
                viewSize.height/2 - mMainWidget->getHeight()/2,
                width,
                mMainWidget->getHeight());

        // by default, the text edit field has the focus of the keyboard
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mItemEdit);

        mSlider->setScrollPosition(maxCount-1);

        mItemEdit->setMinValue(1);
        mItemEdit->setMaxValue(maxCount);
        mItemEdit->setValue(maxCount);
    }

    void CountDialog::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        setVisible(false);
    }

    void CountDialog::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        eventOkClicked(nullptr, mSlider->getScrollPosition()+1);

        setVisible(false);
    }

    // essentially duplicating what the OK button does if user presses
    // Enter key
    void CountDialog::onEnterKeyPressed(MyGUI::EditBox* _sender)
    {
        eventOkClicked(nullptr, mSlider->getScrollPosition()+1);
        setVisible(false);

        // To do not spam onEnterKeyPressed() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void CountDialog::onEditValueChanged(int value)
    {
        mSlider->setScrollPosition(value-1);
    }

    void CountDialog::onSliderMoved(MyGUI::ScrollBar* _sender, size_t _position)
    {
        mItemEdit->setValue(_position+1);
    }
}
