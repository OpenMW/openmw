#ifndef OPENMW_GUI_SPELLVIEW_H
#define OPENMW_GUI_SPELLVIEW_H

#include <boost/tuple/tuple.hpp>

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

        /// simplified update called each frame
        void incrementalUpdate();

        typedef MyGUI::delegates::CMultiDelegate1<SpellModel::ModelIndex> EventHandle_ModelIndex;
        /// Fired when a spell was clicked
        EventHandle_ModelIndex eventSpellClicked;

        virtual void initialiseOverride();

        virtual void setSize(const MyGUI::IntSize& _value);
        virtual void setCoord(const MyGUI::IntCoord& _value);

    private:
        MyGUI::ScrollView* mScrollView;

        std::auto_ptr<SpellModel> mModel;

        /// tracks an item in the spell view
        /// element<0> is the left column GUI object (usually holds the name)
        /// element<1> is the right column (charge or cost info)
        /// element<2> is if line needs to be checked during incremental update
        typedef boost::tuple<MyGUI::Widget*, MyGUI::Widget*, bool> LineInfo;

        std::vector< LineInfo > mLines;

        bool mShowCostColumn;
        bool mHighlightSelected;

        void layoutWidgets();
        void addGroup(const std::string& label1, const std::string& label2);
        void adjustSpellWidget(const Spell& spell, SpellModel::ModelIndex index, MyGUI::Widget* widget);

        void onSpellSelected(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);

        SpellModel::ModelIndex getSpellModelIndex(MyGUI::Widget* _sender);

        static const char* sSpellModelIndex;
    };

}

#endif
