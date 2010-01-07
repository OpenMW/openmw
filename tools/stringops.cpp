#include "stringops.h"

#include <string.h>

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
