#ifndef FILE_FINDER_LESS_H
#define FILE_FINDER_LESS_H

#include <libs/platform/strings.h>
#include <string>

namespace FileFinder{

// Used for maps of file paths. Compares file paths, but ignores case
// AND treats \ and / as the same character.
struct path_less
{
  int compareChar(char a, char b) const
  {
    if(a>b) return 1;
    else if(a<b) return -1;
    return 0;
  }

  int comparePathChar(char a, char b) const
  {
    if(a >= 'a' && a <= 'z') a += 'A'-'a';
    else if(a == '\\') a = '/';
    if(b >= 'a' && b <= 'z') b += 'A'-'a';
    else if(b == '\\') b = '/';
    return compareChar(a,b);
  }

  int compareString(const char *a, const char *b) const
  {
    while(*a && *b)
      {
        int i = comparePathChar(*a,*b);
        if(i != 0) return i;
        a++; b++;
      }
    // At this point, one or both of the chars is a null terminator.
    // Normal char comparison will get the correct final result here.
    return compareChar(*a,*b);
  }

  bool operator() (const std::string& a, const std::string& b) const
  {
    return compareString(a.c_str(), b.c_str()) < 0;
  }
};

struct path_slash
{
  int compareChar(char a, char b) const
  {
    if(a>b) return 1;
    else if(a<b) return -1;
    return 0;
  }

  int comparePathChar(char a, char b) const
  {
   if(a == '\\') a = '/';
   if(b == '\\') b = '/';
    return compareChar(a,b);
  }

  int compareString(const char *a, const char *b) const
  {
    while(*a && *b)
      {
        int i = comparePathChar(*a,*b);
        if(i != 0) return i;
        a++; b++;
      }
    // At this point, one or both of the chars is a null terminator.
    // Normal char comparison will get the correct final result here.
    return compareChar(*a,*b);
  }

  bool operator() (const std::string& a, const std::string& b) const
  {
    return compareString(a.c_str(), b.c_str()) < 0;
  }
};
 
}
#endif
