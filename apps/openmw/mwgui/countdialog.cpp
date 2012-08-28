#include "countdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWGui
{
    CountDialog::CountDialog(MWBase::WindowManager& parWindowManager) :
        WindowModal("openmw_count_window.layout", parWindowManager)
    {
        getWidget(mSlider, "CountSlider");
        getWidget(mItemEdit, "ItemEdit");
        getWidget(mItemText, "ItemText");
        getWidget(mLabelText, "LabelText");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CountDialog::onCancelButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CountDialog::onOkButtonClicked);
        mItemEdit->eventEditTextChange += MyGUI::newDelegate(this, &CountDialog::onEditTextChange);
        mSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &CountDialog::onSliderMoved);
    }

    void CountDialog::open(const std::string& item, const std::string& message, const int maxCount)
    {
        setVisible(true);

        mLabelText->setCaption(message);

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        mSlider->setScrollRange(maxCount);
        mItemText->setCaption(item);

        int width = std::max(mItemText->getTextSize().width + 128, 320);
        setCoord(viewSize.width/2 - width/2,
                viewSize.height/2 - mMainWidget->getHeight()/2,
                width,
                mMainWidget->getHeight());

        MyGUI::InputManager::getInstance().setKeyFocusWidget(mItemEdit);

        mSlider->setScrollPosition(maxCount-1);
        mItemEdit->setCaption(boost::lexical_cast<std::string>(maxCount));

        int okButtonWidth = mOkButton->getTextSize().width + 24;
        mOkButton->setCoord(width - 30 - okButtonWidth,
                            mOkButton->getTop(),
                            okButtonWidth,
                            mOkButton->getHeight());

        int cancelButtonWidth = mCancelButton->getTextSize().width + 24;
        mCancelButton->setCoord(width - 30 - okButtonWidth - cancelButtonWidth - 8,
                            mCancelButton->getTop(),
                            cancelButtonWidth,
                            mCancelButton->getHeight());
    }

    void CountDialog::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        setVisible(false);
    }

    void CountDialog::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        eventOkClicked(NULL, mSlider->getScrollPosition()+1);

        setVisible(false);
    }

    void CountDialog::onEditTextChange(MyGUI::EditBox* _sender)
    {
        if (_sender->getCaption() == "")
            return;

        unsigned int count;
        try
        {
            count = boost::lexical_cast<unsigned int>(_sender->getCaption());
        }
        catch (std::bad_cast&)
        {
            count = 1;
        }
        if (count > mSlider->getScrollRange())
        {
            count = mSlider->getScrollRange();
        }
        mSlider->setScrollPosition(count-1);
        onSliderMoved(mSlider, count-1);
    }

    void CountDialog::onSliderMoved(MyGUI::ScrollBar* _sender, size_t _position)
    {
        mItemEdit->setCaption(boost::lexical_cast<std::string>(_position+1));
    }
}
