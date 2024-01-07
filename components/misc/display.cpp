#include "display.hpp"

#include <numeric>
#include <string>

#include <components/misc/strings/format.hpp>

namespace Misc
{
    std::string getResolutionText(int x, int y, const std::string& format)
    {
        int gcd = std::gcd(x, y);
        if (gcd == 0)
            return std::string();

        int xaspect = x / gcd;
        int yaspect = y / gcd;

        // It is unclear how to handle 90-degree screen rotation properly.
        // So far only swap aspects, apply custom formatting logic and then swap back.
        // As result, 1920 x 1200 is displayed as "1200 x 1920 (10:16)"
        bool flipped = false;
        if (yaspect > xaspect)
        {
            flipped = true;
            std::swap(xaspect, yaspect);
        }

        // 683:384 (used in 1366 x 768) is usually referred as 16:9
        if (xaspect == 683 && yaspect == 384)
        {
            xaspect = 16;
            yaspect = 9;
        }
        // 85:48 (used in 1360 x 768) is usually referred as 16:9
        else if (xaspect == 85 && yaspect == 48)
        {
            xaspect = 16;
            yaspect = 9;
        }
        // 49:36 (used in 1176 x 864) is usually referred as 4:3
        else if (xaspect == 49 && yaspect == 36)
        {
            xaspect = 4;
            yaspect = 3;
        }
        // 39:29 (used in 624 x 484) is usually referred as 4:3
        else if (xaspect == 39 && yaspect == 29)
        {
            xaspect = 4;
            yaspect = 3;
        }
        // 8:5 (used in 1440 x 900) is usually referred as 16:10
        else if (xaspect == 8 && yaspect == 5)
        {
            xaspect = 16;
            yaspect = 10;
        }
        // 5:3 (used in 1280 x 768) is usually referred as 15:9
        else if (xaspect == 5 && yaspect == 3)
        {
            xaspect = 15;
            yaspect = 9;
        }
        else
        {
            // everything between 21:9 and 22:9
            // is usually referred as 21:9
            float ratio = static_cast<float>(xaspect) / yaspect;
            if (ratio >= 21 / 9.f && ratio < 22 / 9.f)
            {
                xaspect = 21;
                yaspect = 9;
            }
        }

        if (flipped)
            std::swap(xaspect, yaspect);

        return Misc::StringUtils::format(format, x, y, xaspect, yaspect);
    }
}
