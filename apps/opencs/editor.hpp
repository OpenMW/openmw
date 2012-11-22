#ifndef CS_EDITOR_H
#define CS_EDITOR_H

#include "model/doc/documentmanager.hpp"
#include "view/doc/viewmanager.hpp"

namespace CS
{
    class Editor
    {
            CSMDoc::DocumentManager mDocumentManager;
            CSVDoc::ViewManager mViewManager;

            // not implemented
            Editor (const Editor&);
            Editor& operator= (const Editor&);

        public:

            Editor();

            void createDocument();

            int run();
            ///< \return error status
    };
}

#endif