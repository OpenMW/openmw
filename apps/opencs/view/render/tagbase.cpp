
#include "tagbase.hpp"

CSVRender::TagBase::TagBase (Mask mask) : mMask (mask) {}

CSVRender::Mask CSVRender::TagBase::getMask() const
{
    return mMask;
}

QString CSVRender::TagBase::getToolTip (bool hideBasics) const
{
    return "";
}
