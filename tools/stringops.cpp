#include "stringops.h"

/// Returns true if str1 begins with substring str2
bool begins(const char* str1, const char* str2)
{
  while(*str2)
    {
      if(*str1 == 0 || *str1 != *str2) return false;

      str1++;
      str2++;
    }
  return true;
}
