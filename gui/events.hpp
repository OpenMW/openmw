#ifndef OENGINE_MYGUI_EVENTS_H
#define OENGINE_MYGUI_EVENTS_H

#include <mangle/input/event.hpp>

namespace MyGUI
{
  class Gui;
}

namespace OEngine {
namespace GUI
{
  /** Event handler that injects OIS events into MyGUI
   */
  class EventInjector : public Mangle::Input::Event
  {
    MyGUI::Gui *gui;

  public:
    bool enabled;

    EventInjector(MyGUI::Gui *g) : gui(g), enabled(true) {}
    void event(Type type, int index, const void *p);
  };
}}
#endif
