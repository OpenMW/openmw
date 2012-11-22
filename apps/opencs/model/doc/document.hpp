#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <QUndoStack>

namespace CSMDoc
{
    class Document
    {
            QUndoStack mUndoStack;

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

        public:

            Document();

            QUndoStack& getUndoStack();
    };
}

#endif