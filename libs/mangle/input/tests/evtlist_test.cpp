#include <iostream>
#include "../filters/eventlist.hpp"

using namespace std;
using namespace Mangle::Input;

struct MyEvent : Event
{
  int ii;
  MyEvent(int i) : ii(i) {}

  void event(Event::Type type, int i, const void *p)
  {
    cout << "  #" << ii << " got event: type=" << type << " index=" << i << endl;
  }
};

EventList lst;

int iii=1;
void make(int flags)
{
  lst.add(EventPtr(new MyEvent(iii++)), flags);
}

void send(Event::Type type)
{
  cout << "Sending type " << type << endl;
  lst.event(type,0,NULL);
}

int main()
{
  make(Event::EV_ALL);
  make(Event::EV_KeyDown);
  make(Event::EV_KeyUp | Event::EV_MouseDown);

  send(Event::EV_Unknown);
  send(Event::EV_KeyDown);
  send(Event::EV_KeyUp);
  send(Event::EV_MouseDown);

  cout << "Enough of that\n";
  return 0;
}
