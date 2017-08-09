#include "loadordererror.hpp"
#include <cassert>

QString ContentSelectorModel::LoadOrderError::sErrorToolTips[ErrorCode_LoadOrder] =
{
    QString("Unable to find dependent file: %1"),
    QString("Dependent file needs to be active: %1"),
    QString("This file needs to load after %1")
};

QString ContentSelectorModel::LoadOrderError::toolTip() const
{
    assert(mErrorCode);
    return sErrorToolTips[mErrorCode - 1].arg(mFileName);
}
