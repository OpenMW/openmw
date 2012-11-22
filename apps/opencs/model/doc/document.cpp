
#include "document.hpp"

CSMDoc::Document::Document() {}

QUndoStack& CSMDoc::Document::getUndoStack()
{
    return mUndoStack;
}