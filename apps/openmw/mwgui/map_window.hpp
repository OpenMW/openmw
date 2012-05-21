#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include "window_pinnable_base.hpp"

namespace MWGui
{
    class LocalMapBase
    {
    public:
        LocalMapBase();
        void init(MyGUI::ScrollView* widget, OEngine::GUI::Layout* layout);

        void setCellPrefix(const std::string& prefix);
        void setActiveCell(const int x, const int y, bool interior=false);

        void toggleFogOfWar();

    protected:
        int mCurX, mCurY;
        bool mInterior;
        MyGUI::ScrollView* mLocalMap;
        std::string mPrefix;
        bool mChanged;
        bool mFogOfWar;

        void applyFogOfWar();

        OEngine::GUI::Layout* mLayout;

        float mLastPositionX;
        float mLastPositionY;
        float mLastDirectionX;
        float mLastDirectionY;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase
    {
    public:
        MapWindow(WindowManager& parWindowManager);
        virtual ~MapWindow(){}

        void setPlayerPos(const float x, const float y);
        void setPlayerDir(const float x, const float y);
        void setCellName(const std::string& cellName);
  
    private:
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onWorldButtonClicked(MyGUI::Widget* _sender);

        MyGUI::ScrollView* mGlobalMap;
        MyGUI::ImageBox* mPlayerArrow;
        MyGUI::Button* mButton;
        MyGUI::IntPoint mLastDragPos;
        bool mGlobal;

    protected:
        virtual void onPinToggled();
    };
}
#endif
