#ifndef OPENMW_GUI_SPELLVIEW_H
#define OPENMW_GUI_SPELLVIEW_H

#include <MyGUI_Widget.h>

#include "spellmodel.hpp"

namespace MyGUI
{
    class ScrollView;
}

namespace MWGui
{

    class SpellModel;

    ///@brief Displays a SpellModel in a list widget
    class SpellView : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(SpellView)
    public:
        SpellView();

        /// Register needed components with MyGUI's factory manager
        static void registerComponents ();

        /// Should the cost/chance column be shown?
        void setShowCostColumn(bool show);

        void setHighlightSelected(bool highlight);

        /// Takes ownership of \a model
        void setModel (SpellModel* model);

        SpellModel* getModel();

        void update();

        typedef MyGUI::delegates::CMultiDelegate1<SpellModel::ModelIndex> EventHandle_ModelIndex;
        /// Fired when a spell was clicked
        EventHandle_ModelIndex eventSpellClicked;

        virtual void initialiseOverride();

        virtual void setSize(const MyGUI::IntSize& _value);
        virtual void setCoord(const MyGUI::IntCoord& _value);

    private:
        MyGUI::ScrollView* mScrollView;

        std::auto_ptr<SpellModel> mModel;

        std::vector< std::pair<MyGUI::Widget*, MyGUI::Widget*> > mLines;

        bool mShowCostColumn;
        bool mHighlightSelected;

        void layoutWidgets();
        void addGroup(const std::string& label1, const std::string& label2);
        void adjustSpellWidget(const Spell& spell, SpellModel::ModelIndex index, MyGUI::Widget* widget);

        void onSpellSelected(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
    };

}

#endif
