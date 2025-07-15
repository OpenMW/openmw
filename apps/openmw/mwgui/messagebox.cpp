#include "messagebox.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_UString.h>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{

    MessageBoxManager::MessageBoxManager(float timePerChar)
    {
        mStaticMessageBox = nullptr;
        mLastButtonPressed = -1;
        mMessageBoxSpeed = timePerChar;
    }

    MessageBoxManager::~MessageBoxManager()
    {
        MessageBoxManager::clear();
    }

    std::size_t MessageBoxManager::getMessagesCount()
    {
        return mMessageBoxes.size();
    }

    void MessageBoxManager::clear()
    {
        if (mInterMessageBoxe)
        {
            mInterMessageBoxe->setVisible(false);
            mInterMessageBoxe.reset();
        }

        mMessageBoxes.clear();
        mStaticMessageBox = nullptr;

        mLastButtonPressed = -1;
    }

    void MessageBoxManager::resetInteractiveMessageBox()
    {
        if (mInterMessageBoxe)
        {
            mInterMessageBoxe->setVisible(false);
            mInterMessageBoxe.reset();
        }
    }

    void MessageBoxManager::setLastButtonPressed(int index)
    {
        mLastButtonPressed = index;
    }

    void MessageBoxManager::onFrame(float frameDuration)
    {
        for (auto it = mMessageBoxes.begin(); it != mMessageBoxes.end();)
        {
            (*it)->mCurrentTime += frameDuration;
            if ((*it)->mCurrentTime >= (*it)->mMaxTime && it->get() != mStaticMessageBox)
            {
                it = mMessageBoxes.erase(it);
            }
            else
                ++it;
        }

        float height = 0;
        auto it = mMessageBoxes.begin();
        while (it != mMessageBoxes.end())
        {
            (*it)->update(static_cast<int>(height));
            height += (*it)->getHeight();
            ++it;
        }

        if (mInterMessageBoxe != nullptr && mInterMessageBoxe->mMarkedToDelete)
        {
            mLastButtonPressed = mInterMessageBoxe->readPressedButton();
            mInterMessageBoxe->setVisible(false);
            mInterMessageBoxe.reset();
            MWBase::Environment::get().getInputManager()->changeInputMode(
                MWBase::Environment::get().getWindowManager()->isGuiMode());
        }
    }

    void MessageBoxManager::createMessageBox(std::string_view message, bool stat)
    {
        auto box = std::make_unique<MessageBox>(*this, message);
        box->mCurrentTime = 0;
        auto realMessage = MyGUI::LanguageManager::getInstance().replaceTags({ message.data(), message.size() });
        box->mMaxTime = realMessage.length() * mMessageBoxSpeed;

        if (stat)
            mStaticMessageBox = box.get();

        box->setVisible(mVisible);

        mMessageBoxes.push_back(std::move(box));

        if (mMessageBoxes.size() > 3)
        {
            mMessageBoxes.erase(mMessageBoxes.begin());
        }

        int height = 0;
        for (const auto& messageBox : mMessageBoxes)
        {
            messageBox->update(height);
            height += messageBox->getHeight();
        }
    }

    void MessageBoxManager::removeStaticMessageBox()
    {
        removeMessageBox(mStaticMessageBox);
        mStaticMessageBox = nullptr;
    }

    bool MessageBoxManager::createInteractiveMessageBox(
        std::string_view message, const std::vector<std::string>& buttons, bool immediate, int defaultFocus)
    {
        if (mInterMessageBoxe != nullptr)
        {
            Log(Debug::Warning) << "Warning: replacing an interactive message box that was not answered yet";
            mInterMessageBoxe->setVisible(false);
        }

        mInterMessageBoxe
            = std::make_unique<InteractiveMessageBox>(*this, std::string{ message }, buttons, immediate, defaultFocus);
        mLastButtonPressed = -1;

        return true;
    }

    bool MessageBoxManager::isInteractiveMessageBox()
    {
        return mInterMessageBoxe != nullptr;
    }

    bool MessageBoxManager::removeMessageBox(MessageBox* msgbox)
    {
        for (auto it = mMessageBoxes.begin(); it != mMessageBoxes.end(); ++it)
        {
            if (it->get() == msgbox)
            {
                mMessageBoxes.erase(it);
                return true;
            }
        }
        return false;
    }

    const std::vector<std::unique_ptr<MessageBox>>& MessageBoxManager::getActiveMessageBoxes() const
    {
        return mMessageBoxes;
    }

    int MessageBoxManager::readPressedButton(bool reset)
    {
        int pressed = mLastButtonPressed;
        if (reset)
            mLastButtonPressed = -1;
        return pressed;
    }

    void MessageBoxManager::setVisible(bool value)
    {
        mVisible = value;
        for (const auto& messageBox : mMessageBoxes)
            messageBox->setVisible(value);
    }

    MessageBox::MessageBox(MessageBoxManager& parMessageBoxManager, std::string_view message)
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

    void MessageBox::update(int height)
    {
        MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos;
        pos.left = (gameWindowSize.width - mMainWidget->getWidth()) / 2;
        pos.top = (gameWindowSize.height - mMainWidget->getHeight() - height - mBottomPadding);

        mMainWidget->setPosition(pos);
    }

    int MessageBox::getHeight()
    {
        return mMainWidget->getHeight() + mNextBoxPadding;
    }

    void MessageBox::setVisible(bool value)
    {
        mMainWidget->setVisible(value);
    }

    InteractiveMessageBox::InteractiveMessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message,
        const std::vector<std::string>& buttons, bool immediate, size_t defaultFocus)
        : WindowModal(MWBase::Environment::get().getWindowManager()->isGuiMode()
                ? "openmw_interactive_messagebox_notransp.layout"
                : "openmw_interactive_messagebox.layout")
        , mMessageBoxManager(parMessageBoxManager)
        , mButtonPressed(-1)
        , mDefaultFocus(defaultFocus)
        , mImmediate(immediate)
        , mControllerFocus(0)
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

        for (const std::string& buttonId : buttons)
        {
            MyGUI::Button* button = mButtonsWidget->createWidget<MyGUI::Button>(
                MyGUI::WidgetStyle::Child, std::string("MW_Button"), dummyCoord, MyGUI::Align::Default);
            button->setCaptionWithReplacing(buttonId);

            button->eventMouseButtonClick += MyGUI::newDelegate(this, &InteractiveMessageBox::mousePressed);

            mButtons.push_back(button);

            if (buttonsWidth != 0)
                buttonsWidth += buttonLeftPadding;

            int buttonWidth = button->getTextSize().width + 2 * buttonLabelLeftPadding;
            buttonsWidth += buttonWidth;

            buttonHeight = button->getTextSize().height + 2 * buttonLabelTopPadding;

            if (buttonsHeight != 0)
                buttonsHeight += buttonTopPadding;
            buttonsHeight += buttonHeight;

            if (buttonWidth > biggestButtonWidth)
            {
                biggestButtonWidth = buttonWidth;
            }
        }

        if (Settings::gui().mControllerMenus)
        {
            mDisableGamepadCursor = true;
            mControllerButtons.a = "#{Interface:OK}";

            // If we have more than one button, we need to set the focus to the first one.
            if (mButtons.size() > 1)
            {
                mControllerFocus = 0;
                if (mDefaultFocus < mButtons.size())
                    mControllerFocus = mDefaultFocus;
                for (size_t i = 0; i < mButtons.size(); ++i)
                    mButtons[i]->setStateSelected(i == mControllerFocus);
            }
        }

        MyGUI::IntSize mainWidgetSize;
        if (buttonsWidth < textSize.width)
        {
            // on one line
            mainWidgetSize.width = textSize.width + 3 * textPadding;
            mainWidgetSize.height
                = textPadding + textSize.height + textButtonPadding + buttonHeight + buttonMainPadding;

            MyGUI::IntSize realSize = mainWidgetSize +
                // To account for borders
                (mMainWidget->getSize() - mMainWidget->getClientWidget()->getSize());

            MyGUI::IntPoint absPos;
            absPos.left = (gameWindowSize.width - realSize.width) / 2;
            absPos.top = (gameWindowSize.height - realSize.height) / 2;

            mMainWidget->setPosition(absPos);
            mMainWidget->setSize(realSize);

            MyGUI::IntCoord messageWidgetCoord;
            messageWidgetCoord.left = (mainWidgetSize.width - textSize.width) / 2;
            messageWidgetCoord.top = textPadding;
            mMessageWidget->setCoord(messageWidgetCoord);

            mMessageWidget->setSize(textSize);

            MyGUI::IntCoord buttonCord;
            MyGUI::IntSize buttonSize(0, buttonHeight);
            int left = (mainWidgetSize.width - buttonsWidth) / 2;

            for (MyGUI::Button* button : mButtons)
            {
                buttonCord.left = left;
                buttonCord.top = messageWidgetCoord.top + textSize.height + textButtonPadding;

                buttonSize.width = button->getTextSize().width + 2 * buttonLabelLeftPadding;
                buttonSize.height = button->getTextSize().height + 2 * buttonLabelTopPadding;

                button->setCoord(buttonCord);
                button->setSize(buttonSize);

                left += buttonSize.width + buttonLeftPadding;
            }
        }
        else
        {
            // among each other
            if (biggestButtonWidth > textSize.width)
            {
                mainWidgetSize.width = biggestButtonWidth + buttonTopPadding * 2;
            }
            else
            {
                mainWidgetSize.width = textSize.width + 3 * textPadding;
            }

            MyGUI::IntCoord buttonCord;
            MyGUI::IntSize buttonSize(0, buttonHeight);

            int top = textPadding + textSize.height + textButtonPadding;

            for (MyGUI::Button* button : mButtons)
            {
                buttonSize.width = button->getTextSize().width + buttonLabelLeftPadding * 2;
                buttonSize.height = button->getTextSize().height + buttonLabelTopPadding * 2;

                buttonCord.top = top;
                buttonCord.left = (mainWidgetSize.width - buttonSize.width) / 2;

                button->setCoord(buttonCord);
                button->setSize(buttonSize);

                top += buttonSize.height + buttonTopPadding;
            }

            mainWidgetSize.height
                = textPadding + textSize.height + textButtonPadding + buttonsHeight + buttonMainPadding;
            mMainWidget->setSize(mainWidgetSize +
                // To account for borders
                (mMainWidget->getSize() - mMainWidget->getClientWidget()->getSize()));

            MyGUI::IntPoint absPos;
            absPos.left = (gameWindowSize.width - mainWidgetSize.width) / 2;
            absPos.top = (gameWindowSize.height - mainWidgetSize.height) / 2;

            mMainWidget->setPosition(absPos);

            MyGUI::IntCoord messageWidgetCoord;
            messageWidgetCoord.left = (mainWidgetSize.width - textSize.width) / 2;
            messageWidgetCoord.top = textPadding;
            messageWidgetCoord.width = textSize.width;
            messageWidgetCoord.height = textSize.height;
            mMessageWidget->setCoord(messageWidgetCoord);
        }

        setVisible(true);
    }

    MyGUI::Widget* InteractiveMessageBox::getDefaultKeyFocus()
    {
        if (mDefaultFocus < mButtons.size())
            return mButtons[mDefaultFocus];
        auto& languageManager = MyGUI::LanguageManager::getInstance();
        std::vector<MyGUI::UString> keywords{ languageManager.replaceTags("#{sOk}"),
            languageManager.replaceTags("#{sYes}") };

        for (MyGUI::Button* button : mButtons)
        {
            for (const MyGUI::UString& keyword : keywords)
            {
                if (Misc::StringUtils::ciEqual(keyword, button->getCaption()))
                {
                    return button;
                }
            }
        }
        return nullptr;
    }

    void InteractiveMessageBox::mousePressed(MyGUI::Widget* pressed)
    {
        buttonActivated(pressed);
    }

    void InteractiveMessageBox::buttonActivated(MyGUI::Widget* pressed)
    {
        mMarkedToDelete = true;
        int index = 0;
        for (const MyGUI::Button* button : mButtons)
        {
            if (button == pressed)
            {
                mButtonPressed = index;
                mMessageBoxManager.onButtonPressed(mButtonPressed);
                if (!mImmediate)
                    return;

                mMessageBoxManager.setLastButtonPressed(mButtonPressed);
                MWBase::Environment::get().getInputManager()->changeInputMode(
                    MWBase::Environment::get().getWindowManager()->isGuiMode());
                return;
            }
            index++;
        }
    }

    int InteractiveMessageBox::readPressedButton()
    {
        return mButtonPressed;
    }

    bool InteractiveMessageBox::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (!mButtons.empty())
            {
                if (mControllerFocus >= mButtons.size())
                    mControllerFocus = mButtons.size() - 1;
                buttonActivated(mButtons[mControllerFocus]);
            }
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            if (mButtons.size() == 1)
                buttonActivated(mButtons[0]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP || arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            if (mButtons.size() <= 1)
                return true;
            if (mButtons.size() == 2 && mControllerFocus == 0)
                return true;

            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus - 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN || arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            if (mButtons.size() <= 1)
                return true;
            if (mButtons.size() == 2 && mControllerFocus == 1)
                return true;

            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus + 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }

        return true;
    }
}
