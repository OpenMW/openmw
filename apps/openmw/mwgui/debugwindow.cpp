#include "debugwindow.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_TabControl.h>
#include <MyGUI_TabItem.h>

#include <LinearMath/btQuickprof.h>
#include <components/debug/debugging.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

#include <mutex>

#ifndef BT_NO_PROFILE

namespace
{
    void bulletDumpRecursive(CProfileIterator* pit, int spacing, std::stringstream& os)
    {
        pit->First();
        if (pit->Is_Done())
            return;

        float accumulated_time = 0,
              parent_time
            = pit->Is_Root() ? CProfileManager::Get_Time_Since_Reset() : pit->Get_Current_Parent_Total_Time();
        int i, j;
        int frames_since_reset = CProfileManager::Get_Frame_Count_Since_Reset();
        for (i = 0; i < spacing; i++)
            os << ".";
        os << "----------------------------------\n";
        for (i = 0; i < spacing; i++)
            os << ".";
        std::string s = "Profiling: " + std::string(pit->Get_Current_Parent_Name())
            + " (total running time: " + MyGUI::utility::toString(parent_time, 3) + " ms) ---\n";
        os << s;
        // float totalTime = 0.f;

        int numChildren = 0;

        for (i = 0; !pit->Is_Done(); i++, pit->Next())
        {
            numChildren++;
            float current_total_time = pit->Get_Current_Total_Time();
            accumulated_time += current_total_time;
            float fraction = parent_time > SIMD_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;

            for (j = 0; j < spacing; j++)
                os << ".";
            double ms = (current_total_time / (double)frames_since_reset);
            s = MyGUI::utility::toString(i) + " -- " + pit->Get_Current_Name() + " ("
                + MyGUI::utility::toString(fraction, 2) + " %) :: " + MyGUI::utility::toString(ms, 3) + " ms / frame ("
                + MyGUI::utility::toString(pit->Get_Current_Total_Calls()) + " calls)\n";
            os << s;
            // totalTime += current_total_time;
            // recurse into children
        }

        if (parent_time < accumulated_time)
        {
            os << "what's wrong\n";
        }
        for (i = 0; i < spacing; i++)
            os << ".";
        double unaccounted = parent_time > SIMD_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f;
        s = "Unaccounted: (" + MyGUI::utility::toString(unaccounted, 3)
            + " %) :: " + MyGUI::utility::toString(parent_time - accumulated_time, 3) + " ms\n";
        os << s;

        for (i = 0; i < numChildren; i++)
        {
            pit->Enter_Child(i);
            bulletDumpRecursive(pit, spacing + 3, os);
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
        // - Material editor
        // - Shader editor

        MyGUI::TabItem* itemLV = mTabControl->addItem("Log Viewer");
        itemLV->setCaptionWithReplacing(" #{OMWEngine:LogViewer} ");
        mLogView
            = itemLV->createWidgetReal<MyGUI::EditBox>("LogEdit", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Stretch);
        mLogView->setEditReadOnly(true);

        MyGUI::TabItem* itemLuaProfiler = mTabControl->addItem("Lua Profiler");
        itemLuaProfiler->setCaptionWithReplacing(" #{OMWEngine:LuaProfiler} ");
        mLuaProfiler = itemLuaProfiler->createWidgetReal<MyGUI::EditBox>(
            "LogEdit", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Stretch);
        mLuaProfiler->setEditReadOnly(true);

#ifndef BT_NO_PROFILE
        MyGUI::TabItem* item = mTabControl->addItem("Physics Profiler");
        item->setCaptionWithReplacing(" #{OMWEngine:PhysicsProfiler} ");
        mBulletProfilerEdit
            = item->createWidgetReal<MyGUI::EditBox>("LogEdit", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Stretch);
#else
        mBulletProfilerEdit = nullptr;
#endif
    }

    static std::vector<char> sLogCircularBuffer;
    static std::mutex sBufferMutex;
    static int64_t sLogStartIndex;
    static int64_t sLogEndIndex;
    static bool hasPrefix = false;

    void DebugWindow::startLogRecording()
    {
        sLogCircularBuffer.resize(Settings::general().mLogBufferSize);
        Debug::setLogListener([](Debug::Level level, std::string_view prefix, std::string_view msg) {
            if (sLogCircularBuffer.empty())
                return; // Log viewer is disabled.
            std::string_view color;
            switch (level)
            {
                case Debug::Error:
                    color = "#FF0000";
                    break;
                case Debug::Warning:
                    color = "#FFFF00";
                    break;
                case Debug::Info:
                    color = "#FFFFFF";
                    break;
                case Debug::Verbose:
                case Debug::Debug:
                    color = "#666666";
                    break;
                default:
                    color = "#FFFFFF";
            }
            bool bufferOverflow = false;
            std::lock_guard lock(sBufferMutex);
            const int64_t bufSize = sLogCircularBuffer.size();
            auto addChar = [&](char c) {
                sLogCircularBuffer[sLogEndIndex++] = c;
                if (sLogEndIndex == bufSize)
                    sLogEndIndex = 0;
                bufferOverflow = bufferOverflow || sLogEndIndex == sLogStartIndex;
            };
            auto addShieldedStr = [&](std::string_view s) {
                for (char c : s)
                {
                    addChar(c);
                    if (c == '#')
                        addChar(c);
                    if (c == '\n')
                        hasPrefix = false;
                }
            };
            for (char c : color)
                addChar(c);
            if (!hasPrefix)
            {
                addShieldedStr(prefix);
                hasPrefix = true;
            }
            addShieldedStr(msg);
            if (bufferOverflow)
                sLogStartIndex = (sLogEndIndex + 1) % bufSize;
        });
    }

    void DebugWindow::updateLogView()
    {
        std::lock_guard lock(sBufferMutex);

        if (!mLogView || sLogCircularBuffer.empty() || sLogStartIndex == sLogEndIndex)
            return;
        if (mLogView->isTextSelection())
            return; // Don't change text while player is trying to copy something

        std::string addition;
        const int64_t bufSize = sLogCircularBuffer.size();
        {
            if (sLogStartIndex < sLogEndIndex)
                addition = std::string(sLogCircularBuffer.data() + sLogStartIndex, sLogEndIndex - sLogStartIndex);
            else
            {
                addition = std::string(sLogCircularBuffer.data() + sLogStartIndex, bufSize - sLogStartIndex);
                addition.append(sLogCircularBuffer.data(), sLogEndIndex);
            }
            sLogStartIndex = sLogEndIndex;
        }

        size_t scrollPos = mLogView->getVScrollPosition();
        bool scrolledToTheEnd = scrollPos + 1 >= mLogView->getVScrollRange();
        int64_t newSizeEstimation = mLogView->getTextLength() + addition.size();
        if (newSizeEstimation > bufSize)
            mLogView->eraseText(0, newSizeEstimation - bufSize);
        mLogView->addText(addition);
        if (scrolledToTheEnd && mLogView->getVScrollRange() > 0)
            mLogView->setVScrollPosition(mLogView->getVScrollRange() - 1);
        else
            mLogView->setVScrollPosition(scrollPos);
    }

    void DebugWindow::updateLuaProfile()
    {
        if (mLuaProfiler->isTextSelection())
            return;

        size_t previousPos = mLuaProfiler->getVScrollPosition();
        mLuaProfiler->setCaption(MWBase::Environment::get().getLuaManager()->formatResourceUsageStats());
        mLuaProfiler->setVScrollPosition(std::min(previousPos, mLuaProfiler->getVScrollRange() - 1));
    }

    void DebugWindow::updateBulletProfile()
    {
#ifndef BT_NO_PROFILE
        std::stringstream stream;
        bulletDumpAll(stream);

        if (mBulletProfilerEdit->isTextSelection()) // pause updating while user is trying to copy text
            return;

        size_t previousPos = mBulletProfilerEdit->getVScrollPosition();
        mBulletProfilerEdit->setCaption(stream.str());
        mBulletProfilerEdit->setVScrollPosition(std::min(previousPos, mBulletProfilerEdit->getVScrollRange() - 1));
#endif
    }

    void DebugWindow::onFrame(float dt)
    {
        static float timer = 0;
        timer -= dt;
        if (timer > 0 || !isVisible())
            return;
        timer = 0.25;

        switch (mTabControl->getIndexSelected())
        {
            case 0:
                updateLogView();
                break;
            case 1:
                updateLuaProfile();
                break;
            case 2:
                updateBulletProfile();
                break;
            default:;
        }
    }
}
