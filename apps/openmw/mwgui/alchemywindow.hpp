#ifndef MWGUI_ALCHEMY_H
#define MWGUI_ALCHEMY_H

#include <vector>

#include "../mwmechanics/alchemy.hpp"

#include "windowbase.hpp"

namespace MWMechanics
{
    class Alchemy;
}

namespace MWGui
{
    class ItemView;
    class ItemWidget;
    class SortFilterItemModel;

    class AlchemyWindow : public WindowBase
    {
    public:
        AlchemyWindow();

        virtual void onOpen();

        void onResChange(int, int) { center(); }

    private:
        std::string mSuggestedPotionName;

        ItemView* mItemView;
        SortFilterItemModel* mSortModel;

        MyGUI::Button* mCreateButton;
        MyGUI::Button* mCancelButton;

        MyGUI::Widget* mEffectsBox;

        MyGUI::EditBox* mNameEdit;

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onCreateButtonClicked(MyGUI::Widget* _sender);
        void onIngredientSelected(MyGUI::Widget* _sender);
        void onAccept(MyGUI::EditBox*);

        void onSelectedItem(int index);

        void removeIngredient(MyGUI::Widget* ingredient);

        void update();

        std::unique_ptr<MWMechanics::Alchemy> mAlchemy;

        std::vector<ItemWidget*> mApparatus;
        std::vector<ItemWidget*> mIngredients;
    };
}

#endif
