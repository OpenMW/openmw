#ifdef _WIN32

#include "win32.hpp"

#undef  _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>

#include <windows.h>

namespace Debug {

bool attachParentConsole()
{
    if (GetConsoleWindow() != nullptr)
        return true;

    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        fflush(stdout);
        fflush(stderr);
        std::cout.flush();
        std::cerr.flush();

        // this looks dubious but is really the right way
        _wfreopen(L"CON", L"w", stdout);
        _wfreopen(L"CON", L"w", stderr);
        _wfreopen(L"CON", L"r", stdin);
        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
        freopen("CON", "r", stdin);

        return true;
    }

    return false;
}

} // ns Debug

#endif
