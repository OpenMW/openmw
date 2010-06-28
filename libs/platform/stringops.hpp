#ifndef __STRINGOPS_H
#define __STRINGOPS_H

namespace OMW { namespace Platform {

    /// Returns true if str1 begins with substring str2
    bool begins(const char* str1, const char* str2);

    /// Returns true if str1 ends with substring str2
    bool ends(const char* str1, const char* str2);

    /// Case insensitive, returns true if str1 begins with substring str2
    bool ibegins(const char* str1, const char* str2);

    /// Case insensitive, returns true if str1 ends with substring str2
    bool iends(const char* str1, const char* str2);
}}

#endif
