#ifndef MWGUI_SPELLSWINDOW_H
#define MWGUI_SPELLSWINDOW_H

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
    class SpellsWindow : public ContainerBase, public WindowBase
    {
        public:
            SpellsWindow(MWBase::WindowManager& parWindowManager);

            void startSpells(MWWorld::Ptr actor);

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mSpells;
            MyGUI::TextBox* mSelect;

            MyGUI::WidgetPtr mSpellsBoxWidget, mSpellsClientWidget;
            MyGUI::ScrollBar* mSpellsScrollerWidget;

            std::map<MyGUI::Widget*, const ESM::Spell*> mSpellsWidgetMap;
            std::map<const ESM::Spell*, int> mSpellsPriceMap;
            std::vector<MyGUI::WidgetPtr> mSpellsWidgets;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);
            void onFocusLost(MyGUI::Widget* _sender, MyGUI::Widget* _old);
            void onSpellButtonClick(MyGUI::Widget* _sender);
            void addSpell(std::string spellID);
            void clearSpells();
            void updateScroller();
            void onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            int mLastPos,mCurrentY;

            static const int sLineHeight;

            // don't show items that the NPC has equipped in his trade-window.
            virtual bool ignoreEquippedItems() { return true; }

            virtual bool isTrading() { return true; }
            virtual bool isTradeWindow() { return true; }

            void updateLabels();

            virtual void onReferenceUnavailable();
    };
}

#endif
