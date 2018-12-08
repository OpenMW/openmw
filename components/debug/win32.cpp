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
    // we already have a console window
    if (GetConsoleWindow() != nullptr)
        return true;

    // our parent window has a console we can use
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // start with consistent state
        fflush(stdin);
        fflush(stderr);
        std::cout.flush();
        std::cerr.flush();

        // fix fprintf(3) and fwprintf(3)
        // this looks strange, but nothing is ever simple on Windows.
        _wfreopen(L"CON", L"w", stdout);
        _wfreopen(L"CON", L"w", stderr);
        _wfreopen(L"CON", L"r", stdin);
        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
        freopen("CON", "w", stderr);

        // it can be verified that input/output works as expected.
#if 0
        fprintf(stdout, "ascii stdout\n");
        fwprintf(stdout, L"wide stdout\n");
        fprintf(stderr, "ascii stderr\n");
        fwprintf(stderr, L"wide stderr\n");

        std::cout << "ascii cout\n";
        std::cout << L"wide cout\n";
        std::cerr << "ascii cerr\n";
        std::cerr << L"wide cerr\n";
#endif

        return true;
    }

    return false;
}

} // ns Debug

#endif
