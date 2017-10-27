#ifndef CSM_DOC_SAVING_H
#define CSM_DOC_SAVING_H

#include <experimental/filesystem>

#include <components/to_utf8/to_utf8.hpp>

#include "operation.hpp"
#include "savingstate.hpp"



namespace CSMDoc
{
    class Document;

    class Saving : public Operation
    {
            Q_OBJECT

            Document& mDocument;
            SavingState mState;

        public:

            Saving (Document& document, const std::experimental::filesystem::path& projectPath,
                ToUTF8::FromType encoding);

    };
}

#endif
