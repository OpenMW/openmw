#ifndef CSM_DOC_SAVING_H
#define CSM_DOC_SAVING_H

#include <QObject>

#include <components/toutf8/toutf8.hpp>

#include "operation.hpp"
#include "savingstate.hpp"

#include <filesystem>

namespace CSMDoc
{
    class Document;

    class Saving : public Operation
    {
        Q_OBJECT

        Document& mDocument;
        SavingState mState;

    public:
        Saving(Document& document, const std::filesystem::path& projectPath, ToUTF8::FromType encoding);
    };
}

#endif
