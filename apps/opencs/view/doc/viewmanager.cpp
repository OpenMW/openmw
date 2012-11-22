
#include "viewmanager.hpp"

#include "view.hpp"

CSVDoc::ViewManager::ViewManager()
{

}

CSVDoc::ViewManager::~ViewManager()
{
    for (std::vector<View *>::iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
        delete *iter;
}

CSVDoc::View *CSVDoc::ViewManager::addView (CSMDoc::Document *document)
{
    View *view = new View (document);

    mViews.push_back (view);

    view->show();

    return view;
}