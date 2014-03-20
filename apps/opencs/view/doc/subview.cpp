#include "subview.hpp"

CSVDoc::SubView::SubView (const CSMWorld::UniversalId& id) : mUniversalId (id)
{
    /// \todo  add a button to the title bar that clones this sub view

    setWindowTitle (mUniversalId.toString().c_str());
}

CSMWorld::UniversalId CSVDoc::SubView::getUniversalId() const
{
    return mUniversalId;
}

void CSVDoc::SubView::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
}

void CSVDoc::SubView::setStatusBar (bool show) {}

void CSVDoc::SubView::useHint (const std::string& hint) {}

void CSVDoc::SubView::setUniversalId (const CSMWorld::UniversalId& id)
{
    mUniversalId = id;
    setWindowTitle (mUniversalId.toString().c_str());
}