#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include "layouts.hpp"
#include "window_pinnable_base.hpp"

namespace MWGui
{
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
        float mLastPositionX;
        float mLastPositionY;
        float mLastDirectionX;
        float mLastDirectionY;
    };
}
#endif
