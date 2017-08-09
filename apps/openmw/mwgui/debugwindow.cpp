#include "debugwindow.hpp"

#include <MyGUI_TabControl.h>
#include <MyGUI_TabItem.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_EditBox.h>

#include <LinearMath/btQuickprof.h>

#ifndef BT_NO_PROFILE

namespace
{
    void bulletDumpRecursive(CProfileIterator* pit, int spacing, std::stringstream& os)
    {
        pit->First();
        if (pit->Is_Done())
            return;

        float accumulated_time=0,parent_time = pit->Is_Root() ? CProfileManager::Get_Time_Since_Reset() : pit->Get_Current_Parent_Total_Time();
        int i,j;
        int frames_since_reset = CProfileManager::Get_Frame_Count_Since_Reset();
        for (i=0;i<spacing;i++) os << ".";
        os << "----------------------------------\n";
        for (i=0;i<spacing;i++) os << ".";
        std::string s = "Profiling: "+
                std::string(pit->Get_Current_Parent_Name())+" (total running time: "+MyGUI::utility::toString(parent_time,3)+" ms) ---\n";
        os << s;
        //float totalTime = 0.f;

        int numChildren = 0;

        for (i = 0; !pit->Is_Done(); i++,pit->Next())
        {
            numChildren++;
            float current_total_time = pit->Get_Current_Total_Time();
            accumulated_time += current_total_time;
            float fraction = parent_time > SIMD_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;

            for (j=0;j<spacing;j++) os << ".";
            double ms = (current_total_time / (double)frames_since_reset);
            s = MyGUI::utility::toString(i)+" -- "+pit->Get_Current_Name()+" ("+MyGUI::utility::toString(fraction,2)+" %) :: "+MyGUI::utility::toString(ms,3)+" ms / frame ("+MyGUI::utility::toString(pit->Get_Current_Total_Calls())+" calls)\n";
            os << s;
            //totalTime += current_total_time;
            //recurse into children
        }

        if (parent_time < accumulated_time)
        {
            os << "what's wrong\n";
        }
        for (i=0;i<spacing;i++) os << ".";
        double unaccounted=  parent_time > SIMD_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f;
        s = "Unaccounted: ("+MyGUI::utility::toString(unaccounted,3)+" %) :: "+MyGUI::utility::toString(parent_time - accumulated_time,3)+" ms\n";
        os << s;

        for (i=0;i<numChildren;i++)
        {
            pit->Enter_Child(i);
            bulletDumpRecursive(pit, spacing+3, os);
            pit->Enter_Parent();
        }
    }

    void bulletDumpAll(std::stringstream& os)
    {
        CProfileIterator* profileIterator = 0;
        profileIterator = CProfileManager::Get_Iterator();

        bulletDumpRecursive(profileIterator, 0, os);

        CProfileManager::Release_Iterator(profileIterator);
    }
}

#endif // BT_NO_PROFILE

namespace MWGui
{

    DebugWindow::DebugWindow()
        : WindowBase("openmw_debug_window.layout")
    {
        getWidget(mTabControl, "TabControl");

        // Ideas for other tabs:
        // - Texture / compositor texture viewer
        // - Log viewer
        // - Material editor
        // - Shader editor

        MyGUI::TabItem* item = mTabControl->addItem("Physics Profiler");
        mBulletProfilerEdit = item->createWidgetReal<MyGUI::EditBox>
                ("LogEdit", MyGUI::FloatCoord(0,0,1,1), MyGUI::Align::Stretch);

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        mMainWidget->setSize(viewSize);


    }

    void DebugWindow::onFrame(float dt)
    {
#ifndef BT_NO_PROFILE
        if (!isVisible())
            return;

        static float timer = 0;
        timer -= dt;

        if (timer > 0)
            return;
        timer = 1;

        std::stringstream stream;
        bulletDumpAll(stream);

        if (mBulletProfilerEdit->isTextSelection()) // pause updating while user is trying to copy text
            return;

        size_t previousPos = mBulletProfilerEdit->getVScrollPosition();
        mBulletProfilerEdit->setCaption(stream.str());
        mBulletProfilerEdit->setVScrollPosition(std::min(previousPos, mBulletProfilerEdit->getVScrollRange()-1));
#endif
    }

}
