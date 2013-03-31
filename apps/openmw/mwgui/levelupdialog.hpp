#ifndef MWGUI_LEVELUPDIALOG_H
#define MWGUI_LEVELUPDIALOG_H

#include "window_base.hpp"

namespace MWGui
{

    class LevelupDialog : public WindowBase
    {
    public:
        LevelupDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

    private:
        MyGUI::Button* mOkButton;
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox* mLevelText;
        MyGUI::EditBox* mLevelDescription;

        MyGUI::Widget* mCoinBox;

        std::vector<MyGUI::Button*> mAttributes;
        std::vector<MyGUI::TextBox*> mAttributeValues;
        std::vector<MyGUI::TextBox*> mAttributeMultipliers;
        std::vector<MyGUI::ImageBox*> mCoins;

        std::vector<int> mSpentAttributes;

        void onOkButtonClicked (MyGUI::Widget* sender);
        void onAttributeClicked (MyGUI::Widget* sender);

        void assignCoins();
        void resetCoins();

        void setAttributeValues();
    };

}

#endif
