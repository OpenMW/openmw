#include <iostream>
using namespace std;

#include "../event_dispatcher.hpp"

using namespace Input;

void callback(int in, int out, void *p)
{
  cout << "    Got event: in=" << in << " out=" << out << endl;
}

EventDispatcher dsp;

void callAll()
{
  cout << "\nDuty calls:\n";
  for(int i=1; i<5; i++)
    {
      cout << "  Calling event " << i << ":\n";
      dsp.call(i);
    }
}

int main()
{
  cout << "Testing the event dispatcher\n";

  dsp.setCallback(&callback);

  callAll();

  dsp.bind(2,1);
  dsp.bind(1,10);
  dsp.bind(14,-12);
  dsp.bind(2,-137);

  callAll();

  dsp.unbind(1,8);
  dsp.unbind(1,10);
  dsp.unbind(2,-137);
  dsp.unbind(2,1);

  callAll();

  dsp.bind(3, 19);
  dsp.bind(4, 18);
  dsp.bind(4, 18);

  callAll();

  return 0;
}
