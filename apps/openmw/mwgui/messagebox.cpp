#include <components/misc/stringops.hpp>

#include "messagebox.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/inputmanager.hpp"

namespace MWGui
{

    MessageBoxManager::MessageBoxManager (float timePerChar)
    {
        mInterMessageBoxe = NULL;
        mStaticMessageBox = NULL;
        mLastButtonPressed = -1;
        mMessageBoxSpeed = timePerChar;
    }

    MessageBoxManager::~MessageBoxManager ()
    {
        std::vector<MessageBox*>::iterator it(mMessageBoxes.begin());
        for (; it != mMessageBoxes.end(); ++it)
        {
            delete *it;
        }
    }

    void MessageBoxManager::clear()
    {
        delete mInterMessageBoxe;
        mInterMessageBoxe = NULL;

        std::vector<MessageBox*>::iterator it(mMessageBoxes.begin());
        for (; it != mMessageBoxes.end(); ++it)
        {
            if (*it == mStaticMessageBox)
                mStaticMessageBox = NULL;
            delete *it;
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
                (*it)->update(height);
                height += (*it)->getHeight();
                ++it;
        }

        if(mInterMessageBoxe != NULL && mInterMessageBoxe->mMarkedToDelete) {
            mLastButtonPressed = mInterMessageBoxe->readPressedButton();
            delete mInterMessageBoxe;
            mInterMessageBoxe = NULL;
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
        for(std::vector<MessageBox*>::iterator it = mMessageBoxes.begin(); it != mMessageBoxes.end(); ++it)
        {
            (*it)->update(height);
            height += (*it)->getHeight();
        }
    }

    void MessageBoxManager::removeStaticMessageBox ()
    {
        removeMessageBox(mStaticMessageBox);
        mStaticMessageBox = NULL;
    }

    bool MessageBoxManager::createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons)
    {
        if(mInterMessageBoxe != NULL) {
            throw std::runtime_error("There is a message box already");
        }

        mInterMessageBoxe = new InteractiveMessageBox(*this, message, buttons);
        mLastButtonPressed = -1;

        return true;
    }

    bool MessageBoxManager::isInteractiveMessageBox ()
    {
        return mInterMessageBoxe != NULL;
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

    void MessageBoxManager::setMessageBoxSpeed (int speed)
    {
        mMessageBoxSpeed = speed;
    }

    int MessageBoxManager::readPressedButton ()
    {
        int pressed = mLastButtonPressed;
        mLastButtonPressed = -1;
        return pressed;
    }




    MessageBox::MessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message)
      : Layout("openmw_messagebox.layout")
      , mMessageBoxManager(parMessageBoxManager)
      , mMessage(message)
      , mCurrentTime(0)
      , mMaxTime(0)
    {
        // defines
        mBottomPadding = 20;
        mNextBoxPadding = 20;

        getWidget(mMessageWidget, "message");

        mMessageWidget->setOverflowToTheLeft(true);
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
        return mMainWidget->getHeight()+mNextBoxPadding; // 20 is the padding between this and the next MessageBox
    }



    InteractiveMessageBox::InteractiveMessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message, const std::vector<std::string>& buttons)
        : WindowModal("openmw_interactive_messagebox.layout")
      , mMessageBoxManager(parMessageBoxManager)
      , mButtonPressed(-1)
        , mTextButtonPadding(0)
    {
        WindowModal::open();

        int textPadding = 10; // padding between text-widget and main-widget
        int textButtonPadding = 20; // padding between the text-widget und the button-widget
        int buttonLeftPadding = 10; // padding between the buttons if horizontal
        int buttonTopPadding = 5; // ^-- if vertical
        int buttonPadding = 5; // padding between button label and button itself
        int buttonMainPadding = 10; // padding between buttons and bottom of the main widget

        mMarkedToDelete = false;


        getWidget(mMessageWidget, "message");
        getWidget(mButtonsWidget, "buttons");

        mMessageWidget->setOverflowToTheLeft(true);
        mMessageWidget->setCaptionWithReplacing(message);

        MyGUI::IntSize textSize = mMessageWidget->getTextSize();

        MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();

        int biggestButtonWidth = 0;
        int buttonWidth = 0;
        int buttonsWidth = 0;
        int buttonHeight = 0;
        MyGUI::IntCoord dummyCoord(0, 0, 0, 0);

        std::vector<std::string>::const_iterator it;
        for(it = buttons.begin(); it != buttons.end(); ++it)
        {
            MyGUI::Button* button = mButtonsWidget->createWidget<MyGUI::Button>(
                MyGUI::WidgetStyle::Child,
                std::string("MW_Button"),
                dummyCoord,
                MyGUI::Align::Default);
            button->setCaptionWithReplacing(*it);

            button->eventMouseButtonClick += MyGUI::newDelegate(this, &InteractiveMessageBox::mousePressed);

            mButtons.push_back(button);

            buttonWidth = button->getTextSize().width + 2*buttonPadding + buttonLeftPadding;
            buttonsWidth += buttonWidth;
            buttonHeight = button->getTextSize().height + 2*buttonPadding + buttonTopPadding;

            if(buttonWidth > biggestButtonWidth)
            {
                biggestButtonWidth = buttonWidth;
            }
        }
        buttonsWidth += buttonLeftPadding;

        MyGUI::IntSize mainWidgetSize;
        // among each other
        if(biggestButtonWidth > textSize.width) {
            mainWidgetSize.width = biggestButtonWidth + buttonTopPadding;
        }
        else {
            mainWidgetSize.width = textSize.width + 3*textPadding;
        }

        MyGUI::IntCoord buttonCord;
        MyGUI::IntSize buttonSize(0, buttonHeight);

        int top = textButtonPadding + buttonTopPadding + textSize.height;

        std::vector<MyGUI::Button*>::const_iterator button;
        for(button = mButtons.begin(); button != mButtons.end(); ++button)
        {
            buttonSize.width = (*button)->getTextSize().width + buttonPadding*2;
            buttonSize.height = (*button)->getTextSize().height + buttonPadding*2;

            buttonCord.top = top;
            buttonCord.left = (mainWidgetSize.width - buttonSize.width)/2 - 5; // FIXME: -5 is not so nice :/

            (*button)->setCoord(buttonCord);
            (*button)->setSize(buttonSize);

            top += buttonSize.height + 2*buttonTopPadding;
        }

        mainWidgetSize.height = top + buttonMainPadding;
        mMainWidget->setSize(mainWidgetSize);

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

        // Set key focus to "Ok" button
        std::string ok = Misc::StringUtils::lowerCase(MyGUI::LanguageManager::getInstance().replaceTags("#{sOK}"));
        for(button = mButtons.begin(); button != mButtons.end(); ++button)
        {
            if(Misc::StringUtils::ciEqual((*button)->getCaption(), ok))
            {
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(*button);
                (*button)->eventKeyButtonPressed += MyGUI::newDelegate(this, &InteractiveMessageBox::onKeyPressed);
                break;
            }
        }
    }

    void InteractiveMessageBox::onKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
    {
        if (_key == MyGUI::KeyCode::Return || _key == MyGUI::KeyCode::NumpadEnter || _key == MyGUI::KeyCode::Space)
            buttonActivated(_sender);
    }

    void InteractiveMessageBox::mousePressed (MyGUI::Widget* pressed)
    {
        buttonActivated (pressed);
    }

    void InteractiveMessageBox::buttonActivated (MyGUI::Widget* pressed)
    {
        mMarkedToDelete = true;
        int index = 0;
        std::vector<MyGUI::Button*>::const_iterator button;
        for(button = mButtons.begin(); button != mButtons.end(); ++button)
        {
            if(*button == pressed)
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
