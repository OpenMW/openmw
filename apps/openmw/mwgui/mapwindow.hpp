#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include "windowpinnablebase.hpp"
#include <cstdint>

namespace MWRender
{
    class GlobalMap;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWGui
{
    class LocalMapBase
    {
    public:
        LocalMapBase();
        virtual ~LocalMapBase();
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

        void onMarkerFocused(MyGUI::Widget* w1, MyGUI::Widget* w2);
        void onMarkerUnfocused(MyGUI::Widget* w1, MyGUI::Widget* w2);

        MyGUI::IntPoint getMarkerPosition (float worldX, float worldY, MarkerPosition& markerPos);

        virtual void notifyPlayerUpdate() {}
        virtual void notifyMapChanged() {}

        // Update markers (Detect X effects, Mark/Recall effects)
        // Note, door markers handled in setActiveCell
        void updateMarkers();
        void addDetectionMarkers(int type);

        OEngine::GUI::Layout* mLayout;

        bool mMapDragAndDrop;

        float mLastPositionX;
        float mLastPositionY;
        float mLastDirectionX;
        float mLastDirectionY;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase, public NoDrop
    {
    public:
        MapWindow(DragAndDrop* drag, const std::string& cacheDir);
        virtual ~MapWindow();

        void setCellName(const std::string& cellName);

        void renderGlobalMap(Loading::Listener* loadingListener);

        void addVisitedLocation(const std::string& name, int x, int y); // adds the marker to the global map
        void cellExplored(int x, int y);

        void setGlobalMapPlayerPosition (float worldX, float worldY);

        virtual void open();

        void onFrame(float dt) { NoDrop::onFrame(dt); }

        /// Clear all savegame-specific data
        void clear();

        void write (ESM::ESMWriter& writer);
        void readRecord (ESM::ESMReader& reader, int32_t type);

    private:
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onWorldButtonClicked(MyGUI::Widget* _sender);

        void globalMapUpdatePlayer();

        MyGUI::ScrollView* mGlobalMap;
        MyGUI::ImageBox* mGlobalMapImage;
        MyGUI::ImageBox* mGlobalMapOverlay;
        MyGUI::ImageBox* mPlayerArrowLocal;
        MyGUI::ImageBox* mPlayerArrowGlobal;
        MyGUI::Button* mButton;
        MyGUI::IntPoint mLastDragPos;
        bool mGlobal;

        MyGUI::Button* mEventBoxGlobal;
        MyGUI::Button* mEventBoxLocal;

        MWRender::GlobalMap* mGlobalMapRender;

    protected:
        virtual void onPinToggled();

        virtual void notifyPlayerUpdate();
        virtual void notifyMapChanged();

    };
}
#endif
