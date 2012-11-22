#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <QUndoStack>
#include <QObject>

namespace CSMDoc
{
    class Document : public QObject
    {
            Q_OBJECT

        public:

            enum State
            {
                    State_Modified = 1
            };

            QUndoStack mUndoStack;

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

        public:

            Document();

            QUndoStack& getUndoStack();

            int getState() const;

        signals:

            void stateChanged (int state, CSMDoc::Document *document);

        private slots:

            void modificationStateChanged (bool clean);
    };
}

#endif