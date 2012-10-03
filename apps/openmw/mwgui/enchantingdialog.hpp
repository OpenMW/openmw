#ifndef MWGUI_ENCHANTINGDIALOG_H
#define MWGUI_ENCHANTINGDIALOG_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include "spellcreationdialog.hpp"

#include "../mwbase/windowmanager.hpp"

namespace MWGui
{

    class EnchantingDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        EnchantingDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();
        void startEnchanting(MWWorld::Ptr actor);

    protected:
        virtual void onReferenceUnavailable();

        void onCancelButtonClicked(MyGUI::Widget* sender);

        MyGUI::Button* mCancelButton;
    };

}

#endif
