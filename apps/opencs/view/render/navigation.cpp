
#include "navigation.hpp"

float CSVRender::Navigation::getFactor (bool mouse) const
{
    float factor = mFastModeFactor;

    if (mouse)
        factor /= 2; /// \todo make this configurable

    return factor;
}

CSVRender::Navigation::Navigation()
    : mFastModeFactor(1)
{
}

CSVRender::Navigation::~Navigation() {}

void CSVRender::Navigation::setFastModeFactor (float factor)
{
    mFastModeFactor = factor;
}
