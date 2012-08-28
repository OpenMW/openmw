#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include "window_pinnable_base.hpp"

namespace MWGui
{
    class LocalMapBase
    {
    public:
        LocalMapBase();
        void init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, OEngine::GUI::Layout* layout, bool mapDragAndDrop=false);

        void setCellPrefix(const std::string& prefix);
        void setActiveCell(const int x, const int y, bool interior=false);
        void setPlayerDir(const float x, const float y);
        void setPlayerPos(const float x, const float y);

        void toggleFogOfWar();

        struct MarkerPosition
        {
            bool interior;
            int cellX;
            int cellY;
            float nX;
            float nY;
        };

    protected:
        int mCurX, mCurY;
        bool mInterior;
        MyGUI::ScrollView* mLocalMap;
        MyGUI::ImageBox* mCompass;
        std::string mPrefix;
        bool mChanged;
        bool mFogOfWar;

        std::vector<MyGUI::ImageBox*> mMapWidgets;
        std::vector<MyGUI::ImageBox*> mFogWidgets;

        void applyFogOfWar();

        OEngine::GUI::Layout* mLayout;

        bool mMapDragAndDrop;

        float mLastPositionX;
        float mLastPositionY;
        float mLastDirectionX;
        float mLastDirectionY;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase
    {
    public:
        MapWindow(MWBase::WindowManager& parWindowManager);
        virtual ~MapWindow(){}

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
