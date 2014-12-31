#include "loadordererror.hpp"
#include <assert.h>

QString ContentSelectorModel::LoadOrderError::sErrorToolTips[ErrorCode_LoadOrder] =
{
    QString("Unable to find dependant file: %1"),
    QString("Dependent file needs to be active: %1"),
    QString("This file needs to load after %1")
};

ContentSelectorModel::LoadOrderError ContentSelectorModel::LoadOrderError::sNoError = ContentSelectorModel::LoadOrderError();

QString ContentSelectorModel::LoadOrderError::toolTip() const
{
    assert(mErrorCode);
    return sErrorToolTips[mErrorCode - 1].arg(mFileName);
}

bool ContentSelectorModel::LoadOrderError::operator== (const ContentSelectorModel::LoadOrderError& rhs) const
{
    return (mErrorCode == rhs.mErrorCode) && ((mErrorCode == ErrorCode_None) || (mFileName == rhs.mFileName));
}