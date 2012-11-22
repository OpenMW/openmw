
#include "viewmanager.hpp"

#include <map>

#include "../../model/doc/documentmanager.hpp"

#include "view.hpp"

void CSVDoc::ViewManager::updateIndices()
{
    std::map<CSMDoc::Document *, std::pair<int, int> > documents;

    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
    {
        std::map<CSMDoc::Document *, std::pair<int, int> >::iterator document = documents.find ((*iter)->getDocument());

        if (document==documents.end())
            document =
                documents.insert (
                    std::make_pair ((*iter)->getDocument(), std::make_pair (0, countViews ((*iter)->getDocument())))).
                first;

        (*iter)->setIndex (document->second.first++, document->second.second);
    }
}

CSVDoc::ViewManager::ViewManager (CSMDoc::DocumentManager& documentManager)
: mDocumentManager (documentManager)
{

}

CSVDoc::ViewManager::~ViewManager()
{
    for (std::vector<View *>::iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
        delete *iter;

    for (std::vector<View *>::iterator iter (mClosed.begin()); iter!=mClosed.end(); ++iter)
        delete *iter;
}

CSVDoc::View *CSVDoc::ViewManager::addView (CSMDoc::Document *document)
{
    View *view = new View (*this, document, countViews (document)+1);

    mViews.push_back (view);

    view->show();

    updateIndices();

    return view;
}

int CSVDoc::ViewManager::countViews (const CSMDoc::Document *document) const
{
    int count = 0;

    for (std::vector<View *>::const_iterator iter (mViews.begin()); iter!=mViews.end(); ++iter)
        if ((*iter)->getDocument()==document)
            ++count;

    return count;
}

bool CSVDoc::ViewManager::closeRequest (View *view)
{
    std::vector<View *>::iterator iter = std::find (mViews.begin(), mViews.end(), view);

    if (iter!=mViews.end())
    {
        bool last = countViews (view->getDocument())<=1;

        /// \todo check if document has not been saved -> return false and start close dialogue

        mViews.erase (iter);
        mClosed.push_back (view);

        if (last)
            mDocumentManager.removeDocument (view->getDocument());
        else
            updateIndices();
    }

    return true;
}