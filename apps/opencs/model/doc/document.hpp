#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <QUndoStack>
#include <QObject>
#include <QTimer>

namespace CSMDoc
{
    class Document : public QObject
    {
            Q_OBJECT

        public:

            enum State
            {
                    State_Modified = 1,
                    State_Locked = 2,
                    State_Saving = 4
            };

            QUndoStack mUndoStack;

            int mSaveCount; ///< dummy implementation -> remove when proper save is implemented.
            QTimer mSaveTimer; ///< dummy implementation -> remove when proper save is implemented.

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

        public:

            Document();

            QUndoStack& getUndoStack();

            int getState() const;

            void save();

            void abortOperation (int type);

        signals:

            void stateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

        private slots:

            void modificationStateChanged (bool clean);

            void saving();
            ///< dummy implementation -> remove when proper save is implemented.
    };
}

#endif