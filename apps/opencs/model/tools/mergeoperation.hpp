#ifndef CSM_TOOLS_MERGEOPERATION_H
#define CSM_TOOLS_MERGEOPERATION_H

#include <boost/filesystem/path.hpp>

#include "../doc/operation.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMTools
{
    class MergeOperation : public CSMDoc::Operation
    {


        public:

            MergeOperation (CSMDoc::Document& document);

            /// \attention Do not call this function while a merge is running.
            void setTarget (const boost::filesystem::path& target);
    };
}

#endif
