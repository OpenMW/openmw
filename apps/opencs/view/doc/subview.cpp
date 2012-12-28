
#include "subview.hpp"

CSVDoc::SubView::SubView (const CSMWorld::UniversalId& id) : mUniversalId (id)
{
    /// \todo  add a button to the title bar that clones this sub view

    setWindowTitle (mUniversalId.toString().c_str());

    /// \todo remove (for testing only)
    setMinimumWidth (100);
    setMinimumHeight (60);
}

CSMWorld::UniversalId CSVDoc::SubView::getUniversalId() const
{
    return mUniversalId;
}
