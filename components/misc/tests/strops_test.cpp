#include <cassert>

#include "../stringops.hpp"

int main()
{
  assert(Misc::begins("abc", "a"));
  assert(Misc::begins("abc", "ab"));
  assert(Misc::begins("abc", "abc"));
  assert(Misc::begins("abcd", "abc"));

  assert(!Misc::begins("abc", "b"));
  assert(!Misc::begins("abc", "bc"));
  assert(!Misc::begins("abc", "bcd"));
  assert(!Misc::begins("abc", "abcd"));

  assert(Misc::ibegins("Abc", "a"));
  assert(Misc::ibegins("aBc", "ab"));
  assert(Misc::ibegins("abC", "abc"));
  assert(Misc::ibegins("abcD", "abc"));

  assert(!Misc::ibegins("abc", "b"));
  assert(!Misc::ibegins("abc", "bc"));
  assert(!Misc::ibegins("abc", "bcd"));
  assert(!Misc::ibegins("abc", "abcd"));

  assert(Misc::ends("abc", "c"));
  assert(Misc::ends("abc", "bc"));
  assert(Misc::ends("abc", "abc"));
  assert(Misc::ends("abcd", "abcd"));

  assert(!Misc::ends("abc", "b"));
  assert(!Misc::ends("abc", "ab"));
  assert(!Misc::ends("abc", "bcd"));
  assert(!Misc::ends("abc", "abcd"));

  assert(Misc::iends("Abc", "c"));
  assert(Misc::iends("aBc", "bc"));
  assert(Misc::iends("abC", "abc"));
  assert(Misc::iends("abcD", "abcd"));

  assert(!Misc::iends("abc", "b"));
  assert(!Misc::iends("abc", "ab"));
  assert(!Misc::iends("abc", "bcd"));
  assert(!Misc::iends("abc", "abcd"));

  return 0;
}
