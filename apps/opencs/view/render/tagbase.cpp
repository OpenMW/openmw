
#include "tagbase.hpp"

CSVRender::TagBase::TagBase (SceneUtil::VisMask mask) : mMask (mask) {}

SceneUtil::VisMask CSVRender::TagBase::getMask() const
{
    return mMask;
}

QString CSVRender::TagBase::getToolTip (bool hideBasics) const
{
    return "";
}
