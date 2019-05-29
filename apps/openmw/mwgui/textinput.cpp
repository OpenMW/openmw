#include "textinput.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#ifdef __SWITCH__
#include <switch.h>
static std::string OnScreenKeyboard(std::string title, std::string init)
{
    SwkbdConfig kbd;
    Result rc;
    static char tmp_out[4096];

    tmp_out[0] = 0;

    rc = swkbdCreate(&kbd, 0);
    if (R_SUCCEEDED(rc))
    {
      swkbdConfigMakePresetDefault(&kbd);
      swkbdConfigSetInitialText(&kbd, init.c_str());
      swkbdConfigSetGuideText(&kbd, title.c_str());
      rc = swkbdShow(&kbd, tmp_out, sizeof(tmp_out));
      swkbdClose(&kbd);
      if (R_SUCCEEDED(rc))
        return std::string(tmp_out);
    }

    return std::string("");
}
#endif

namespace MWGui
{

    TextInputDialog::TextInputDialog()
      : WindowModal("openmw_text_input.layout")
    {
        // Centre dialog
        center();

        getWidget(mTextEdit, "TextEdit");
        mTextEdit->eventEditSelectAccept += newDelegate(this, &TextInputDialog::onTextAccepted);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);

        // Make sure the edit box has focus
        MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
    }

    void TextInputDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    void TextInputDialog::setTextLabel(const std::string &label)
    {
        setText("LabelT", label);
    }

    void TextInputDialog::onOpen()
    {
        WindowModal::onOpen();
        // Make sure the edit box has focus
        MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
#ifdef __SWITCH__
        MyGUI::Widget* pt = nullptr;
        std::string s;
        getWidget(pt, "LabelT");
        if (pt)
            s = OnScreenKeyboard(static_cast<MyGUI::TextBox*>(pt)->getCaption(), mTextEdit->getCaption());
        else
            s = OnScreenKeyboard("", mTextEdit->getCaption());
        setTextInput(s);
#endif
    }

    // widget controls

    void TextInputDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if (mTextEdit->getCaption() == "")
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage37}");
            MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
        }
        else
            eventDone(this);
    }

    void TextInputDialog::onTextAccepted(MyGUI::Edit* _sender)
    {
        onOkClicked(_sender);

        // To do not spam onTextAccepted() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    std::string TextInputDialog::getTextInput() const
    {
        return mTextEdit->getCaption();
    }

    void TextInputDialog::setTextInput(const std::string &text)
    {
        mTextEdit->setCaption(text);
    }


}
