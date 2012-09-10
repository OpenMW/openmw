#ifndef MWGUI_SpellBuyingWINDOW_H
#define MWGUI_SpellBuyingWINDOW_H

#include "container.hpp"
#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{
    class SpellBuyingWindow : public ContainerBase, public WindowBase
    {
        public:
            SpellBuyingWindow(MWBase::WindowManager& parWindowManager);

            void startSpellBuying(const MWWorld::Ptr& actor);

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mSpells;
            MyGUI::TextBox* mSelect;

            MyGUI::WidgetPtr mSpellsBoxWidget, mSpellsClientWidget;
            MyGUI::ScrollBar* mSpellsScrollerWidget;

            MWWorld::Ptr mActor;

            std::map<MyGUI::Widget*, std::string> mSpellsWidgetMap;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onSpellButtonClick(MyGUI::Widget* _sender);
            void updateScroller();
            void onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            void addSpell(const std::string& spellID);
            void clearSpells();
            int mLastPos,mCurrentY;

            static const int sLineHeight;

            void updateLabels();

            virtual void onReferenceUnavailable();
    };
}

#endif
