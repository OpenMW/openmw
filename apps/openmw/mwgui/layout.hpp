#ifndef OPENMW_MWGUI_LAYOUT_H
#define OPENMW_MWGUI_LAYOUT_H

#include <string>
#include <MyGUI_WidgetDefines.h>
#include <MyGUI_Widget.h>

#include <components/debug/debuglog.hpp>

namespace MWGui
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
    virtual ~Layout()
    {
        try
        {
            shutdown();
        }
        catch(const MyGUI::Exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
    }

    MyGUI::Widget* getWidget(const std::string& _name);

    template <typename T>
    void getWidget(T * & _widget, const std::string & _name)
    {
        MyGUI::Widget* w = getWidget(_name);
        T* cast = w->castType<T>(false);
        if (!cast)
        {
            MYGUI_EXCEPT("Error cast : dest type = '" << T::getClassTypeName()
                         << "' source name = '" << w->getName()
                         << "' source type = '" << w->getTypeName() << "' in layout '" << mLayoutName << "'");
        }
        else
            _widget = cast;
    }

  private:
    void initialise(const std::string & _layout,
                    MyGUI::Widget* _parent = nullptr);

    void shutdown();

  public:
    void setCoord(int x, int y, int w, int h);

    virtual void setVisible(bool b);

    void setText(const std::string& name, const std::string& caption);

    // NOTE: this assume that mMainWidget is of type Window.
    void setTitle(const std::string& title);

    MyGUI::Widget* mMainWidget;

  protected:

    std::string mPrefix;
    std::string mLayoutName;
    MyGUI::VectorWidgetPtr mListWindowRoot;
  };
}
#endif
