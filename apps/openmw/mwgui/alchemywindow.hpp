#ifndef MWGUI_ALCHEMY_H
#define MWGUI_ALCHEMY_H

#include <vector>

#include "../mwmechanics/alchemy.hpp"

#include "widgets.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    class ItemView;
    class SortFilterItemModel;

    class AlchemyWindow : public WindowBase
    {
    public:
        AlchemyWindow();

        virtual void open();

    private:
        ItemView* mItemView;
        SortFilterItemModel* mSortModel;

        MyGUI::Button* mCreateButton;
        MyGUI::Button* mCancelButton;

        MyGUI::Widget* mEffectsBox;

        MyGUI::EditBox* mNameEdit;

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onCreateButtonClicked(MyGUI::Widget* _sender);
        void onIngredientSelected(MyGUI::Widget* _sender);

        void onSelectedItem(int index);

        void removeIngredient(MyGUI::Widget* ingredient);

        void update();

        MWMechanics::Alchemy mAlchemy;

        std::vector<MyGUI::ImageBox *> mApparatus;
        std::vector<MyGUI::ImageBox *> mIngredients;
    };
}

#endif
