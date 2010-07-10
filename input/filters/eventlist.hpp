#ifndef MANGLE_INPUT_EVENTLIST_H
#define MANGLE_INPUT_EVENTLIST_H

#include "../event.hpp"
#include <vector>

namespace Mangle
{
  namespace Input
  {
    /** And Event handler that distributes each event to a list of
        other handlers. Supports filtering events by their Type
        parameter.
     */
    struct EventList : Event
    {
      struct Filter
      {
        EventPtr evt;
        int flags;
      };
      std::vector<Filter> list;

      void add(EventPtr e, int flags = EV_ALL)
      {
        Filter f;
        f.evt = e;
        f.flags = flags;
        list.push_back(f);
      }

      virtual void event(Type type, int index, const void *p)
      {
        std::vector<Filter>::iterator it;

        for(it=list.begin(); it!=list.end(); it++)
          {
            if(type & it->flags)
              it->evt->event(type,index,p);
          }
      }
    };
  }
}
#endif
