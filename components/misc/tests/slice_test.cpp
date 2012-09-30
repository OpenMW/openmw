#include <iostream>

using namespace std;

#include "../slice_array.hpp"

int main()
{
  Misc::SString s, t;
  s = Misc::SString("hello");
  cout << s.toString() << ", len=" << s.length << endl;
  cout << (s=="hel") << (s=="hell") << (s=="hello") << endl;
  t = s;

  s = Misc::SString("othello"+2, 4);
  cout << s.toString() << ", len=" << s.length << endl;
  cout << (s=="hel") << (s=="hell") << (s=="hello") << endl;

  cout << (s==t) << (Misc::SString("hello")==t) << endl;

  const int arr[4] = {1,2,3,4};

  Misc::IntArray ia(arr,4);

  cout << ia.length << " " << ia.ptr[2] << endl;

  return 0;
}
