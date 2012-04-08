#ifndef OENGINE_MYGUI_LAYOUT_H
#define OENGINE_MYGUI_LAYOUT_H

#include <assert.h>
#include <MyGUI.h>

namespace OEngine {
namespace GUI
{
  /** The Layout class is an utility class used to load MyGUI layouts
      from xml files, and to manipulate member widgets.
   */
  class Layout
  {
  public:
    Layout(const std::string & _layout, MyGUI::Widget* _parent = nullptr)
      : mMainWidget(nullptr)
    { initialise(_layout, _parent); }
    virtual ~Layout() { shutdown();  }

    template <typename T>
    void getWidget(T * & _widget, const std::string & _name, bool _throw = true)
    {
      _widget = nullptr;
      for (MyGUI::VectorWidgetPtr::iterator iter=mListWindowRoot.begin();
           iter!=mListWindowRoot.end(); ++iter)
        {
          MyGUI::Widget* find = (*iter)->findWidget(mPrefix + _name);
          if (nullptr != find)
            {
              T * cast = find->castType<T>(false);
              if (nullptr != cast)
                _widget = cast;
              else if (_throw)
                {
                  MYGUI_EXCEPT("Error cast : dest type = '" << T::getClassTypeName()
                               << "' source name = '" << find->getName()
                               << "' source type = '" << find->getTypeName() << "' in layout '" << mLayoutName << "'");
                }
              return;
            }
        }
      MYGUI_ASSERT( ! _throw, "widget name '" << _name << "' in layout '" << mLayoutName << "' not found.");
    }

    void initialise(const std::string & _layout,
                    MyGUI::Widget* _parent = nullptr)
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

    void shutdown()
    {
      MyGUI::LayoutManager::getInstance().unloadLayout(mListWindowRoot);
      mListWindowRoot.clear();
    }

    void setCoord(int x, int y, int w, int h)
    {
      mMainWidget->setCoord(x,y,w,h);
    }

    void adjustWindowCaption()
    {
      // adjust the size of the window caption so that all text is visible
      // NOTE: this assumes that mMainWidget is of type Window.
      MyGUI::TextBox* box = static_cast<MyGUI::Window*>(mMainWidget)->getCaptionWidget();
      box->setSize(box->getTextSize().width + 48, box->getSize().height);

      // in order to trigger alignment updates, we need to update the parent
      // mygui doesn't provide a proper way of doing this, so we are just changing size
      box->getParent()->setCoord(MyGUI::IntCoord(
          box->getParent()->getCoord().left,
          box->getParent()->getCoord().top,
          box->getParent()->getCoord().width,
          box->getParent()->getCoord().height+1
      ));
      box->getParent()->setCoord(MyGUI::IntCoord(
          box->getParent()->getCoord().left,
          box->getParent()->getCoord().top,
          box->getParent()->getCoord().width,
          box->getParent()->getCoord().height-1
      ));
    }

    virtual void setVisible(bool b)
    {
      mMainWidget->setVisible(b);
    }

    void setText(const std::string& name, const std::string& caption)
    {
      MyGUI::Widget* pt;
      getWidget(pt, name);
      static_cast<MyGUI::TextBox*>(pt)->setCaption(caption);
    }

    void setTextColor(const std::string& name, float r, float g, float b)
    {
      MyGUI::Widget* pt;
      getWidget(pt, name);
      MyGUI::TextBox *st = dynamic_cast<MyGUI::TextBox*>(pt);
      if(st != NULL)
        st->setTextColour(MyGUI::Colour(b,g,r));
    }

    void setImage(const std::string& name, const std::string& imgName)
    {
      MyGUI::ImageBox* pt;
      getWidget(pt, name);
      pt->setImageTexture(imgName);
    }

  protected:

    MyGUI::Widget* mMainWidget;
    std::string mPrefix;
    std::string mLayoutName;
    MyGUI::VectorWidgetPtr mListWindowRoot;
  };
}}
#endif
