#include <iostream>

using namespace std;

#include <assert.h>

#include "../slice_array.hpp"

int main()
{
  SString s, t;
  s = SString("hello");
  cout << s.toString() << ", len=" << s.length << endl;
  cout << (s=="hel") << (s=="hell") << (s=="hello") << endl;
  t = s;

  s = SString("othello"+2, 4);
  cout << s.toString() << ", len=" << s.length << endl;
  cout << (s=="hel") << (s=="hell") << (s=="hello") << endl;

  cout << (s==t) << (SString("hello")==t) << endl;

  const int arr[4] = {1,2,3,4};

  IntArray ia(arr,4);

  cout << ia.length << " " << ia.ptr[2] << endl;

  return 0;
}
