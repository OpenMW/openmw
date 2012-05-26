#ifndef MWGUI_ALCHEMY_H
#define MWGUI_ALCHEMY_H

#include "window_base.hpp"
#include "container.hpp"

namespace MWGui
{
    class AlchemyWindow : public WindowBase, public ContainerBase
    {
    public:
        AlchemyWindow(WindowManager& parWindowManager);

        virtual void open();

    protected:
        MyGUI::Button* mCreateButton;
        MyGUI::Button* mCancelButton;

        MyGUI::ImageBox* mIngredient1;
        MyGUI::ImageBox* mIngredient2;
        MyGUI::ImageBox* mIngredient3;
        MyGUI::ImageBox* mIngredient4;

        MyGUI::ImageBox* mApparatus1;
        MyGUI::ImageBox* mApparatus2;
        MyGUI::ImageBox* mApparatus3;
        MyGUI::ImageBox* mApparatus4;

        MyGUI::Widget* mEffectsBox;

        MyGUI::EditBox* mNameEdit;

        Widgets::SpellEffectList mEffects; // effects of created potion

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onCreateButtonClicked(MyGUI::Widget* _sender);
        void onIngredientSelected(MyGUI::Widget* _sender);

        virtual void onSelectedItemImpl(MWWorld::Ptr item);
        virtual std::vector<MWWorld::Ptr> itemsToIgnore();

        void removeIngredient(MyGUI::Widget* ingredient);

        virtual void onReferenceUnavailable() { ; }

        void update();
    };
}

#endif
