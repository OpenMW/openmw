#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <string>

#include <boost/filesystem/path.hpp>

#include <QUndoStack>
#include <QObject>
#include <QTimer>

#include "../world/data.hpp"

#include "../tools/tools.hpp"

#include "state.hpp"

class QAbstractItemModel;

namespace ESM
{
    struct GameSetting;
    struct Global;
}

namespace CSMDoc
{
    class Document : public QObject
    {
            Q_OBJECT

        private:

            std::string mName; ///< \todo replace name with ESX list
            CSMWorld::Data mData;
            CSMTools::Tools mTools;

            // It is important that the undo stack is declared last, because on desctruction it fires a signal, that is connected to a slot, that is
            // using other member variables.  Unfortunately this connection is cut only in the QObject destructor, which is way too late.
            QUndoStack mUndoStack;

            int mSaveCount; ///< dummy implementation -> remove when proper save is implemented.
            QTimer mSaveTimer; ///< dummy implementation -> remove when proper save is implemented.

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

            void load (const std::vector<boost::filesystem::path>::const_iterator& begin,
                const std::vector<boost::filesystem::path>::const_iterator& end, bool lastAsModified);
            ///< \param lastAsModified Store the last file in Modified instead of merging it into Base.

            void createBase();

            void addOptionalGmsts();

            void addOptionalGlobals();

            void addOptionalGmst (const ESM::GameSetting& gmst);

            void addOptionalGlobal (const ESM::Global& global);

        public:

            Document (const std::vector<boost::filesystem::path>& files, bool new_);
            ~Document();

            QUndoStack& getUndoStack();

            int getState() const;

            const std::string& getName() const;
            ///< \todo replace with ESX list

            void save();

            CSMWorld::UniversalId verify();

            void abortOperation (int type);

            const CSMWorld::Data& getData() const;

            CSMWorld::Data& getData();

            CSMTools::ReportModel *getReport (const CSMWorld::UniversalId& id);
            ///< The ownership of the returned report is not transferred.

        signals:

            void stateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

        private slots:

            void modificationStateChanged (bool clean);

            void operationDone (int type);

            void saving();
            ///< dummy implementation -> remove when proper save is implemented.

        public slots:

            void progress (int current, int max, int type);
    };
}

#endif
