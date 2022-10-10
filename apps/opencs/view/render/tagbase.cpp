#include "tagbase.hpp"

#include <apps/opencs/view/render/mask.hpp>

CSVRender::TagBase::TagBase(Mask mask)
    : mMask(mask)
{
}

CSVRender::Mask CSVRender::TagBase::getMask() const
{
    return mMask;
}

QString CSVRender::TagBase::getToolTip(bool hideBasics, const WorldspaceHitResult& /*hit*/) const
{
    return "";
}
