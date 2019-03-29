#include "messagebox.hpp"

#include <MyGUI_LanguageManager.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Button.h>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#undef MessageBox

namespace MWGui
{

    MessageBoxManager::MessageBoxManager (float timePerChar)
    {
        mInterMessageBoxe = nullptr;
        mStaticMessageBox = nullptr;
        mLastButtonPressed = -1;
        mMessageBoxSpeed = timePerChar;
    }

    MessageBoxManager::~MessageBoxManager ()
    {
        for (MessageBox* messageBox : mMessageBoxes)
        {
            delete messageBox;
        }
    }

    int MessageBoxManager::getMessagesCount()
    {
        return mMessageBoxes.size();
    }

    void MessageBoxManager::clear()
    {
        if (mInterMessageBoxe)
        {
            mInterMessageBoxe->setVisible(false);

            delete mInterMessageBoxe;
            mInterMessageBoxe = nullptr;
        }

        for (MessageBox* messageBox : mMessageBoxes)
        {
            if (messageBox == mStaticMessageBox)
                mStaticMessageBox = nullptr;
            delete messageBox;
        }
        mMessageBoxes.clear();

        mLastButtonPressed = -1;
    }

    void MessageBoxManager::onFrame (float frameDuration)
    {
        std::vector<MessageBox*>::iterator it;
        for(it = mMessageBoxes.begin(); it != mMessageBoxes.end();)
        {
            (*it)->mCurrentTime += frameDuration;
            if((*it)->mCurrentTime >= (*it)->mMaxTime && *it != mStaticMessageBox)
            {
                delete *it;
                it = mMessageBoxes.erase(it);
            }
            else
                ++it;
        }

        float height = 0;
        it = mMessageBoxes.begin();
        while(it != mMessageBoxes.end())
        {
            (*it)->update(static_cast<int>(height));
            height += (*it)->getHeight();
            ++it;
        }

        if(mInterMessageBoxe != nullptr && mInterMessageBoxe->mMarkedToDelete) {
            mLastButtonPressed = mInterMessageBoxe->readPressedButton();
            mInterMessageBoxe->setVisible(false);
            delete mInterMessageBoxe;
            mInterMessageBoxe = nullptr;
            MWBase::Environment::get().getInputManager()->changeInputMode(
                        MWBase::Environment::get().getWindowManager()->isGuiMode());
        }
    }

    void MessageBoxManager::createMessageBox (const std::string& message, bool stat)
    {
        MessageBox *box = new MessageBox(*this, message);
        box->mCurrentTime = 0;
        std::string realMessage = MyGUI::LanguageManager::getInstance().replaceTags(message);
        box->mMaxTime = realMessage.length()*mMessageBoxSpeed;

        if(stat)
            mStaticMessageBox = box;

        mMessageBoxes.push_back(box);

        if(mMessageBoxes.size() > 3) {
            delete *mMessageBoxes.begin();
            mMessageBoxes.erase(mMessageBoxes.begin());
        }

        int height = 0;
        for (MessageBox* messageBox : mMessageBoxes)
        {
            messageBox->update(height);
            height += messageBox->getHeight();
        }
    }

    void MessageBoxManager::removeStaticMessageBox ()
    {
        removeMessageBox(mStaticMessageBox);
        mStaticMessageBox = nullptr;
    }

    bool MessageBoxManager::createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons)
    {
        if (mInterMessageBoxe != nullptr)
        {
            Log(Debug::Warning) << "Warning: replacing an interactive message box that was not answered yet";
            mInterMessageBoxe->setVisible(false);
            delete mInterMessageBoxe;
            mInterMessageBoxe = nullptr;
        }

        mInterMessageBoxe = new InteractiveMessageBox(*this, message, buttons);
        mLastButtonPressed = -1;

        return true;
    }

    bool MessageBoxManager::isInteractiveMessageBox ()
    {
        return mInterMessageBoxe != nullptr;
    }


    bool MessageBoxManager::removeMessageBox (MessageBox *msgbox)
    {
        std::vector<MessageBox*>::iterator it;
        for(it = mMessageBoxes.begin(); it != mMessageBoxes.end(); ++it)
        {
            if((*it) == msgbox)
            {
                delete (*it);
                mMessageBoxes.erase(it);
                return true;
            }
        }
        return false;
    }

    int MessageBoxManager::readPressedButton (bool reset)
    {
        int pressed = mLastButtonPressed;
        if (reset)
            mLastButtonPressed = -1;
        return pressed;
    }




    MessageBox::MessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message)
      : Layout("openmw_messagebox.layout")
      , mCurrentTime(0)
      , mMaxTime(0)
      , mMessageBoxManager(parMessageBoxManager)
      , mMessage(message)
    {
        // defines
        mBottomPadding = 48;
        mNextBoxPadding = 4;

        getWidget(mMessageWidget, "message");

        mMessageWidget->setCaptionWithReplacing(mMessage);
    }

    void MessageBox::update (int height)
    {
        MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos;
        pos.left = (gameWindowSize.width - mMainWidget->getWidth())/2;
        pos.top = (gameWindowSize.height - mMainWidget->getHeight() - height - mBottomPadding);

        mMainWidget->setPosition(pos);
    }

    int MessageBox::getHeight ()
    {
        return mMainWidget->getHeight()+mNextBoxPadding;
    }



    InteractiveMessageBox::InteractiveMessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message, const std::vector<std::string>& buttons)
        : WindowModal(MWBase::Environment::get().getWindowManager()->isGuiMode() ? "openmw_interactive_messagebox_notransp.layout" : "openmw_interactive_messagebox.layout")
      , mMessageBoxManager(parMessageBoxManager)
      , mButtonPressed(-1)
    {
        int textPadding = 10; // padding between text-widget and main-widget
        int textButtonPadding = 10; // padding between the text-widget und the button-widget
        int buttonLeftPadding = 10; // padding between the buttons if horizontal
        int buttonTopPadding = 10; // ^-- if vertical
        int buttonLabelLeftPadding = 12; // padding between button label and button itself, from left
        int buttonLabelTopPadding = 4; // padding between button label and button itself, from top
        int buttonMainPadding = 10; // padding between buttons and bottom of the main widget

        mMarkedToDelete = false;


        getWidget(mMessageWidget, "message");
        getWidget(mButtonsWidget, "buttons");

        mMessageWidget->setSize(400, mMessageWidget->getHeight());
        mMessageWidget->setCaptionWithReplacing(message);

        MyGUI::IntSize textSize = mMessageWidget->getTextSize();

        MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();

        int biggestButtonWidth = 0;
        int buttonsWidth = 0;
        int buttonsHeight = 0;
        int buttonHeight = 0;
        MyGUI::IntCoord dummyCoord(0, 0, 0, 0);

        for(const std::string& buttonId : buttons)
        {
            MyGUI::Button* button = mButtonsWidget->createWidget<MyGUI::Button>(
                MyGUI::WidgetStyle::Child,
                std::string("MW_Button"),
                dummyCoord,
                MyGUI::Align::Default);
            button->setCaptionWithReplacing(buttonId);

            button->eventMouseButtonClick += MyGUI::newDelegate(this, &InteractiveMessageBox::mousePressed);

            mButtons.push_back(button);

            if (buttonsWidth != 0)
                buttonsWidth += buttonLeftPadding;

            int buttonWidth = button->getTextSize().width + 2*buttonLabelLeftPadding;
            buttonsWidth += buttonWidth;

            buttonHeight = button->getTextSize().height + 2*buttonLabelTopPadding;

            if (buttonsHeight != 0)
                buttonsHeight += buttonTopPadding;
            buttonsHeight += buttonHeight;

            if(buttonWidth > biggestButtonWidth)
            {
                biggestButtonWidth = buttonWidth;
            }
        }

        MyGUI::IntSize mainWidgetSize;
        if(buttonsWidth < textSize.width)
        {
            // on one line
            mainWidgetSize.width = textSize.width + 3*textPadding;
            mainWidgetSize.height = textPadding + textSize.height + textButtonPadding + buttonHeight + buttonMainPadding;

            MyGUI::IntSize realSize = mainWidgetSize +
                    // To account for borders
                    (mMainWidget->getSize() - mMainWidget->getClientWidget()->getSize());

            MyGUI::IntPoint absPos;
            absPos.left = (gameWindowSize.width - realSize.width)/2;
            absPos.top = (gameWindowSize.height - realSize.height)/2;

            mMainWidget->setPosition(absPos);
            mMainWidget->setSize(realSize);

            MyGUI::IntCoord messageWidgetCoord;
            messageWidgetCoord.left = (mainWidgetSize.width - textSize.width)/2;
            messageWidgetCoord.top = textPadding;
            mMessageWidget->setCoord(messageWidgetCoord);

            mMessageWidget->setSize(textSize);

            MyGUI::IntCoord buttonCord;
            MyGUI::IntSize buttonSize(0, buttonHeight);
            int left = (mainWidgetSize.width - buttonsWidth)/2;

            for(MyGUI::Button* button : mButtons)
            {
                buttonCord.left = left;
                buttonCord.top = messageWidgetCoord.top + textSize.height + textButtonPadding;

                buttonSize.width = button->getTextSize().width + 2*buttonLabelLeftPadding;
                buttonSize.height = button->getTextSize().height + 2*buttonLabelTopPadding;

                button->setCoord(buttonCord);
                button->setSize(buttonSize);

                left += buttonSize.width + buttonLeftPadding;
            }
        }
        else
        {
            // among each other
            if(biggestButtonWidth > textSize.width) {
                mainWidgetSize.width = biggestButtonWidth + buttonTopPadding*2;
            }
            else {
                mainWidgetSize.width = textSize.width + 3*textPadding;
            }

            MyGUI::IntCoord buttonCord;
            MyGUI::IntSize buttonSize(0, buttonHeight);

            int top = textPadding + textSize.height + textButtonPadding;

            for(MyGUI::Button* button : mButtons)
            {
                buttonSize.width = button->getTextSize().width + buttonLabelLeftPadding*2;
                buttonSize.height = button->getTextSize().height + buttonLabelTopPadding*2;

                buttonCord.top = top;
                buttonCord.left = (mainWidgetSize.width - buttonSize.width)/2;

                button->setCoord(buttonCord);
                button->setSize(buttonSize);

                top += buttonSize.height + buttonTopPadding;
            }

            mainWidgetSize.height = textPadding + textSize.height + textButtonPadding + buttonsHeight + buttonMainPadding;
            mMainWidget->setSize(mainWidgetSize +
                                 // To account for borders
                                 (mMainWidget->getSize() - mMainWidget->getClientWidget()->getSize()));

            MyGUI::IntPoint absPos;
            absPos.left = (gameWindowSize.width - mainWidgetSize.width)/2;
            absPos.top = (gameWindowSize.height - mainWidgetSize.height)/2;

            mMainWidget->setPosition(absPos);

            MyGUI::IntCoord messageWidgetCoord;
            messageWidgetCoord.left = (mainWidgetSize.width - textSize.width)/2;
            messageWidgetCoord.top = textPadding;
            messageWidgetCoord.width = textSize.width;
            messageWidgetCoord.height = textSize.height;
            mMessageWidget->setCoord(messageWidgetCoord);
        }

        setVisible(true);
    }

    MyGUI::Widget* InteractiveMessageBox::getDefaultKeyFocus()
    {
        std::vector<std::string> keywords { "sOk", "sYes" };
        for(MyGUI::Button* button : mButtons)
        {
            for (const std::string& keyword : keywords)
            {
                if(Misc::StringUtils::ciEqual(MyGUI::LanguageManager::getInstance().replaceTags("#{" + keyword + "}"), button->getCaption()))
                {
                    return button;
                }
            }
        }
        return nullptr;
    }

    void InteractiveMessageBox::mousePressed (MyGUI::Widget* pressed)
    {
        buttonActivated (pressed);
    }

    void InteractiveMessageBox::buttonActivated (MyGUI::Widget* pressed)
    {
        mMarkedToDelete = true;
        int index = 0;
        for(const MyGUI::Button* button : mButtons)
        {
            if(button == pressed)
            {
                mButtonPressed = index;
                mMessageBoxManager.onButtonPressed(mButtonPressed);
                return;
            }
            index++;
        }
    }

    int InteractiveMessageBox::readPressedButton ()
    {
        return mButtonPressed;
    }

}
