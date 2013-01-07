#include "stringops.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <string.h>
#include <libs/platform/strings.h>



namespace Misc
{

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

bool ends(const char* str1, const char* str2)
{
  int len1 = strlen(str1);
  int len2 = strlen(str2);

  if(len1 < len2) return false;

  return strcmp(str2, str1+len1-len2) == 0;
}

// True if the given chars match, case insensitive
static bool icmp(char a, char b)
{
  if(a >= 'A' && a <= 'Z')
    a += 'a' - 'A';
  if(b >= 'A' && b <= 'Z')
    b += 'a' - 'A';

  return a == b;
}

bool ibegins(const char* str1, const char* str2)
{
  while(*str2)
    {
      if(*str1 == 0 || !icmp(*str1,*str2)) return false;

      str1++;
      str2++;
    }
  return true;
}

bool iends(const char* str1, const char* str2)
{
  int len1 = strlen(str1);
  int len2 = strlen(str2);

  if(len1 < len2) return false;

  return strcasecmp(str2, str1+len1-len2) == 0;
}

}
