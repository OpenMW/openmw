#ifndef MISC_GCD_H
#define MISC_GCD_H

namespace Misc
{
    // TODO: replace to the std::gcd() when the C++17 will be available.
    int gcd(int a, int b)
    {
        return b == 0 ? a : gcd(b, a % b);
    }
}

#endif
