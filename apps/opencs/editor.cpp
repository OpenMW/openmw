
#include "editor.hpp"

#include <QtGui/QApplication>

CS::Editor::Editor()
{
}

void CS::Editor::createDocument()
{
    CSMDoc::Document *document = mDocumentManager.addDocument();
    mViewManager.addView (document);
}

int CS::Editor::run()
{
    createDocument();

    return QApplication::exec();
}