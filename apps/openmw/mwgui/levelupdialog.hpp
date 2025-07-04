#ifndef MWGUI_LEVELUPDIALOG_H
#define MWGUI_LEVELUPDIALOG_H

#include <components/esm/attr.hpp>

#include "windowbase.hpp"

namespace MWGui
{

    class LevelupDialog : public WindowBase
    {
    public:
        LevelupDialog();

        void onOpen() override;

        std::string_view getWindowIdForLua() const override { return "LevelUpDialog"; }

    private:
        struct Widgets
        {
            MyGUI::Button* mButton;
            MyGUI::TextBox* mValue;
            MyGUI::TextBox* mMultiplier;
        };
        MyGUI::Button* mOkButton;
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox* mLevelText;
        MyGUI::EditBox* mLevelDescription;

        MyGUI::Widget* mCoinBox;
        MyGUI::ScrollView* mAssignWidget;

        std::map<ESM::Attribute::AttributeID, Widgets> mAttributeWidgets;
        std::vector<MyGUI::ImageBox*> mCoins;

        std::vector<ESM::Attribute::AttributeID> mSpentAttributes;

        unsigned int mCoinCount;

        void onOkButtonClicked(MyGUI::Widget* sender);
        void onAttributeClicked(MyGUI::Widget* sender);

        void assignCoins();
        void resetCoins();

        void setAttributeValues();

        std::string_view getLevelupClassImage(
            const int combatIncreases, const int magicIncreases, const int stealthIncreases);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        std::vector<MyGUI::Button*> mAttributeButtons;
        int mControllerFocus;
    };

}

#endif
