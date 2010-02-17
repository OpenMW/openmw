#include <assert.h>

#include "../stringops.h"

int main()
{
  assert(begins("abc", "a"));
  assert(begins("abc", "ab"));
  assert(begins("abc", "abc"));
  assert(begins("abcd", "abc"));

  assert(!begins("abc", "b"));
  assert(!begins("abc", "bc"));
  assert(!begins("abc", "bcd"));
  assert(!begins("abc", "abcd"));

  assert(ibegins("Abc", "a"));
  assert(ibegins("aBc", "ab"));
  assert(ibegins("abC", "abc"));
  assert(ibegins("abcD", "abc"));

  assert(!ibegins("abc", "b"));
  assert(!ibegins("abc", "bc"));
  assert(!ibegins("abc", "bcd"));
  assert(!ibegins("abc", "abcd"));

  assert(ends("abc", "c"));
  assert(ends("abc", "bc"));
  assert(ends("abc", "abc"));
  assert(ends("abcd", "abcd"));

  assert(!ends("abc", "b"));
  assert(!ends("abc", "ab"));
  assert(!ends("abc", "bcd"));
  assert(!ends("abc", "abcd"));

  assert(iends("Abc", "c"));
  assert(iends("aBc", "bc"));
  assert(iends("abC", "abc"));
  assert(iends("abcD", "abcd"));

  assert(!iends("abc", "b"));
  assert(!iends("abc", "ab"));
  assert(!iends("abc", "bcd"));
  assert(!iends("abc", "abcd"));

  return 0;
}
