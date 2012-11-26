#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <string>

#include <QUndoStack>
#include <QObject>
#include <QTimer>

#include "../world/data.hpp"

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
                    State_Saving = 4,
                    State_Verifying = 8,
                    State_Compiling = 16, // not implemented yet
                    State_Searching = 32 // not implemented yet
            };

        private:

            std::string mName; ///< \todo replace name with ESX list
            QUndoStack mUndoStack;
            CSMWorld::Data mData;

            int mSaveCount; ///< dummy implementation -> remove when proper save is implemented.
            QTimer mSaveTimer; ///< dummy implementation -> remove when proper save is implemented.

            int mVerifyCount; ///< dummy implementation -> remove when proper verify is implemented.
            QTimer mVerifyTimer; ///< dummy implementation -> remove when proper verify is implemented.

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

        public:

            Document (const std::string& name);
            ///< \todo replace name with ESX list

            QUndoStack& getUndoStack();

            int getState() const;

            const std::string& getName() const;
            ///< \todo replace with ESX list

            void save();

            void verify();

            void abortOperation (int type);

            const CSMWorld::Data& getData() const;

            CSMWorld::Data& getData();

        signals:

            void stateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

        private slots:

            void modificationStateChanged (bool clean);

            void saving();
            ///< dummy implementation -> remove when proper save is implemented.

            void verifying();
            ///< dummy implementation -> remove when proper verify is implemented.
    };
}

#endif