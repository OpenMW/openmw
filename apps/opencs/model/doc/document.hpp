#ifndef CSM_DOC_DOCUMENT_H
#define CSM_DOC_DOCUMENT_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>

#include <QUndoStack>
#include <QObject>
#include <QTimer>

#include <components/to_utf8/to_utf8.hpp>

#include "../world/data.hpp"
#include "../world/idcompletionmanager.hpp"

#include "../tools/tools.hpp"

#include "state.hpp"
#include "saving.hpp"
#include "blacklist.hpp"
#include "runner.hpp"
#include "operationholder.hpp"

class QAbstractItemModel;

namespace VFS
{

    class Manager;
}

namespace ESM
{
    struct GameSetting;
    struct Global;
    struct MagicEffect;
}

namespace Files
{
    struct ConfigurationManager;
}

namespace CSMWorld
{
    class ResourcesManager;
}

namespace CSMDoc
{
    class Document : public QObject
    {
            Q_OBJECT

        private:

            const VFS::Manager* mVFS;
            boost::filesystem::path mSavePath;
            std::vector<boost::filesystem::path> mContentFiles;
            bool mNew;
            CSMWorld::Data mData;
            CSMTools::Tools mTools;
            boost::filesystem::path mProjectPath;
            Saving mSavingOperation;
            OperationHolder mSaving;
            boost::filesystem::path mResDir;
            Blacklist mBlacklist;
            Runner mRunner;
            bool mDirty;

            CSMWorld::IdCompletionManager mIdCompletionManager;

            // It is important that the undo stack is declared last, because on desctruction it fires a signal, that is connected to a slot, that is
            // using other member variables.  Unfortunately this connection is cut only in the QObject destructor, which is way too late.
            QUndoStack mUndoStack;

            // not implemented
            Document (const Document&);
            Document& operator= (const Document&);

            void createBase();

            void addGmsts();

            void addOptionalGmsts();

            void addOptionalGlobals();

            void addOptionalMagicEffects();

            void addOptionalGmst (const ESM::GameSetting& gmst);

            void addOptionalGlobal (const ESM::Global& global);

            void addOptionalMagicEffect (const ESM::MagicEffect& effect);

        public:

            Document (const VFS::Manager* vfs, const Files::ConfigurationManager& configuration,
                const std::vector< boost::filesystem::path >& files, bool new_,
                const boost::filesystem::path& savePath, const boost::filesystem::path& resDir,
                ToUTF8::FromType encoding, const CSMWorld::ResourcesManager& resourcesManager,
                const std::vector<std::string>& blacklistedScripts);

            ~Document();

            const VFS::Manager* getVFS() const;

            QUndoStack& getUndoStack();

            int getState() const;

            const boost::filesystem::path& getSavePath() const;

            const boost::filesystem::path& getProjectPath() const;

            const std::vector<boost::filesystem::path>& getContentFiles() const;
            ///< \attention The last element in this collection is the file that is being edited,
            /// but with its original path instead of the save path.

            bool isNew() const;
            ///< Is this a newly created content file?

            void save();

            CSMWorld::UniversalId verify (const CSMWorld::UniversalId& reportId = CSMWorld::UniversalId());

            CSMWorld::UniversalId newSearch();

            void runSearch (const CSMWorld::UniversalId& searchId, const CSMTools::Search& search);

            void runMerge (std::auto_ptr<CSMDoc::Document> target);

            void abortOperation (int type);

            const CSMWorld::Data& getData() const;

            CSMWorld::Data& getData();

            CSMTools::ReportModel *getReport (const CSMWorld::UniversalId& id);
            ///< The ownership of the returned report is not transferred.

            bool isBlacklisted (const CSMWorld::UniversalId& id) const;

            void startRunning (const std::string& profile,
                const std::string& startupInstruction = "");

            void stopRunning();

            QTextDocument *getRunLog();

            CSMWorld::IdCompletionManager &getIdCompletionManager();

            void flagAsDirty();

        signals:

            void stateChanged (int state, CSMDoc::Document *document);

            void progress (int current, int max, int type, int threads, CSMDoc::Document *document);

            /// \attention When this signal is emitted, *this hands over the ownership of the
            /// document. This signal must be handled to avoid a leak.
            void mergeDone (CSMDoc::Document *document);

        private slots:

            void modificationStateChanged (bool clean);

            void reportMessage (const CSMDoc::Message& message, int type);

            void operationDone (int type, bool failed);

            void runStateChanged();

        public slots:

            void progress (int current, int max, int type);
    };
}

#endif
