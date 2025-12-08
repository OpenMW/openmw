#include "layout.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_LayoutManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_UString.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Window.h>

namespace MWGui
{
    void Layout::initialise(std::string_view layout)
    {
        constexpr char mainWindow[] = "_Main";
        mLayoutName = layout;

        mPrefix = MyGUI::utility::toString(this, "_");
        mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix);

        const std::string mainName = mPrefix + mainWindow;
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            if (widget->getName() == mainName)
                mMainWidget = widget;

            // Force the alignment to update immediately
            widget->_setAlign(widget->getSize(), widget->getParentSize());
        }
        MYGUI_ASSERT(
            mMainWidget, "root widget name '" << mainWindow << "' in layout '" << mLayoutName << "' not found.");
    }

    void Layout::shutdown()
    {
        setVisible(false);
        MyGUI::Gui::getInstance().destroyWidget(mMainWidget);
        mListWindowRoot.clear();
    }

    void Layout::setCoord(int x, int y, int w, int h)
    {
        mMainWidget->setCoord(x, y, w, h);
    }

    void Layout::setVisible(bool b)
    {
        mMainWidget->setVisible(b);
    }

    void Layout::setText(std::string_view name, std::string_view caption)
    {
        MyGUI::Widget* pt;
        getWidget(pt, name);
        static_cast<MyGUI::TextBox*>(pt)->setCaption(MyGUI::UString(caption));
    }

    void Layout::setTitle(std::string_view title)
    {
        MyGUI::Window* window = static_cast<MyGUI::Window*>(mMainWidget);

        if (window->getCaption() != title)
            window->setCaptionWithReplacing(MyGUI::UString(title));
    }

    MyGUI::Widget* Layout::getWidget(std::string_view name) const
    {
        std::string target = mPrefix;
        target += name;
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            MyGUI::Widget* find = widget->findWidget(target);
            if (nullptr != find)
            {
                return find;
            }
        }
        MYGUI_EXCEPT("widget name '" << name << "' in layout '" << mLayoutName << "' not found.");
    }

}
