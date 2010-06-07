#include <iostream>
using namespace std;

#include "../func_binder.hpp"

void f1(int i, void *p)
{
  cout << "  F1 i=" << i << endl;

  if(p)
    cout << "  Got a nice gift: "
         << *((float*)p) << endl;
}

void f2(int i, void *p)
{
  cout << "  F2 i=" << i << endl;
}

using namespace Input;

int main()
{
  cout << "This will test the function binding system\n";

  FuncBinder bnd(5);

  bnd.bind(0, &f1, "This is action 1");
  bnd.bind(1, &f2);
  bnd.bind(2, &f1, "This is action 3");
  bnd.bind(3, &f2, "This is action 4");

  bnd.unbind(2);

  for(int i=0; i<5; i++)
    {
      cout << "Calling " << i << ": '" << bnd.getName(i) << "'\n";
      bnd.call(i);
      if(!bnd.isBound(i)) cout << "  (not bound)\n";
    }

  cout << "\nCalling with parameter:\n";
  float f = 3.1415;
  bnd.call(0, &f);

  return 0;
}
