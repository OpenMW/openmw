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

  return 0;
}
