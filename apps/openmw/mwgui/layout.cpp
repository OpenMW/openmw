#include "layout.hpp"

#include <MyGUI_LayoutManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Window.h>

namespace MWGui
{
    void Layout::initialise(const std::string& _layout, MyGUI::Widget* _parent)
    {
        const std::string MAIN_WINDOW = "_Main";
        mLayoutName = _layout;

        if (mLayoutName.empty())
            mMainWidget = _parent;
        else
        {
            mPrefix = MyGUI::utility::toString(this, "_");
            mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix, _parent);

            const std::string main_name = mPrefix + MAIN_WINDOW;
            for (MyGUI::VectorWidgetPtr::iterator iter=mListWindowRoot.begin(); iter!=mListWindowRoot.end(); ++iter)
            {
                if ((*iter)->getName() == main_name)
                {
                    mMainWidget = (*iter);
                    break;
                }
            }
            MYGUI_ASSERT(mMainWidget, "root widget name '" << MAIN_WINDOW << "' in layout '" << mLayoutName << "' not found.");
        }
    }

    void Layout::shutdown()
    {
        MyGUI::Gui::getInstance().destroyWidget(mMainWidget);
        mListWindowRoot.clear();
    }

    void Layout::setCoord(int x, int y, int w, int h)
    {
        mMainWidget->setCoord(x,y,w,h);
    }

    void Layout::setVisible(bool b)
    {
        mMainWidget->setVisible(b);
    }

    void Layout::setText(const std::string &name, const std::string &caption)
    {
        MyGUI::Widget* pt;
        getWidget(pt, name);
        static_cast<MyGUI::TextBox*>(pt)->setCaption(caption);
    }

    void Layout::setTitle(const std::string& title)
    {
        MyGUI::Window* window = static_cast<MyGUI::Window*>(mMainWidget);

        if (window->getCaption() != title)
            window->setCaptionWithReplacing(title);
    }

    MyGUI::Widget* Layout::getWidget(const std::string &_name)
    {
        for (MyGUI::VectorWidgetPtr::iterator iter=mListWindowRoot.begin();
        iter!=mListWindowRoot.end(); ++iter)
        {
            MyGUI::Widget* find = (*iter)->findWidget(mPrefix + _name);
            if (nullptr != find)
            {
                return find;
            }
        }
        MYGUI_EXCEPT("widget name '" << _name << "' in layout '" << mLayoutName << "' not found.");
    }

}
