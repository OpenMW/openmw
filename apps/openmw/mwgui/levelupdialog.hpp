#ifndef MWGUI_LEVELUPDIALOG_H
#define MWGUI_LEVELUPDIALOG_H

#include "windowbase.hpp"

namespace MWGui
{

    class LevelupDialog : public WindowBase
    {
    public:
        LevelupDialog();

        void onOpen() override;

    private:
        MyGUI::Button* mOkButton;
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox* mLevelText;
        MyGUI::EditBox* mLevelDescription;

        MyGUI::Widget* mCoinBox;
        MyGUI::Widget* mAssignWidget;

        std::vector<MyGUI::Button*> mAttributes;
        std::vector<MyGUI::TextBox*> mAttributeValues;
        std::vector<MyGUI::TextBox*> mAttributeMultipliers;
        std::vector<MyGUI::ImageBox*> mCoins;

        std::vector<int> mSpentAttributes;

        unsigned int mCoinCount;
        static const unsigned int sMaxCoins;

        void onOkButtonClicked(MyGUI::Widget* sender);
        void onAttributeClicked(MyGUI::Widget* sender);

        void assignCoins();
        void resetCoins();

        void setAttributeValues();

        std::string getLevelupClassImage(const int combatIncreases, const int magicIncreases, const int stealthIncreases);
    };

}

#endif
