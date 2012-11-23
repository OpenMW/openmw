
#include "editor.hpp"

#include <sstream>

#include <QtGui/QApplication>

CS::Editor::Editor() : mViewManager (mDocumentManager), mNewDocumentIndex (0)
{
    connect (&mViewManager, SIGNAL (newDocumentRequest ()), this, SLOT (createDocument ()));
}

void CS::Editor::createDocument()
{
    std::ostringstream stream;

    stream << "NewDocument" << (++mNewDocumentIndex);

    CSMDoc::Document *document = mDocumentManager.addDocument (stream.str());
    mViewManager.addView (document);
}

int CS::Editor::run()
{
    /// \todo Instead of creating an empty document, open a small welcome dialogue window with buttons for new/load/recent projects
    createDocument();

    return QApplication::exec();
}