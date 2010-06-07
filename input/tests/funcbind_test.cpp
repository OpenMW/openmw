#include <iostream>
using namespace std;

#include <string>
#include <vector>
#include <boost/function.hpp>
#include <assert.h>

typedef boost::function<void()> Action;

struct FuncBind
{
  std::string name;
  Action action;
};

class Binder
{
  std::vector<FuncBind> bindings;

public:
  /**
     Initialize the struct by telling it how many functions you have
     to bind. The largest index you intend to use should be number-1.
  */
  Binder(int number) : bindings(number) {}

  void bind(int index, Action action, const std::string &name="")
  {
    assert(index >= 0 && index < bindings.size());
    FuncBind &fb = bindings[index];
    fb.action = action;
    fb.name = name;
  }

  void unbind(int index)
  {
    assert(index >= 0 && index < bindings.size());
    FuncBind &fb = bindings[index];
    fb = FuncBind();
  }

  void call(int index)
  {
    assert(index >= 0 && index < bindings.size());
    FuncBind &fb = bindings[index];
    if(fb.action)
      {
        cout << "Calling '" << fb.name << "'\n";
        fb.action();
      }
    else
      cout << "No function\n";
  }
};

void f1()
{
  cout << "In f1()\n";
}

void f2()
{
  cout << "In f2()\n";
}

int main()
{
  cout << "This will test the function binding system\n";

  Binder bnd(5);

  bnd.bind(0, &f1, "This is action 1");
  bnd.bind(1, &f2);
  bnd.bind(2, &f1, "This is action 3");

  for(int i=0; i<5; i++)
    {
      cout << "\nCalling " << i << endl;
      bnd.call(i);
    }

  return 0;
}
