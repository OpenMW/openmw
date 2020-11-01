#ifndef CSV_DOC_SUBVIEW_H
#define CSV_DOC_SUBVIEW_H

#include "../../model/doc/document.hpp"

#include "../../model/world/universalid.hpp"

#include "subviewfactory.hpp"

#include <QDockWidget>

class QUndoStack;

namespace CSMWorld
{
    class Data;
}

namespace CSVDoc
{
    class View;

    class SubView : public QDockWidget
    {
            Q_OBJECT

            CSMWorld::UniversalId mUniversalId;

            // not implemented
            SubView (const SubView&);
            SubView& operator= (SubView&);

        protected:

            void setUniversalId(const CSMWorld::UniversalId& id);

            bool event (QEvent *event) override;

        public:

            SubView (const CSMWorld::UniversalId& id);

            CSMWorld::UniversalId getUniversalId() const;

            virtual void setEditLock (bool locked) = 0;

            virtual void setStatusBar (bool show);
            ///< Default implementation: ignored

            virtual void useHint (const std::string& hint);
            ///< Default implementation: ignored

            virtual std::string getTitle() const;

        private:

            void closeEvent (QCloseEvent *event) override;

        signals:

            void focusId (const CSMWorld::UniversalId& universalId, const std::string& hint);

            void closeRequest (SubView *subView);

            void updateTitle();

            void updateSubViewIndices (SubView *view = nullptr);

            void universalIdChanged (const CSMWorld::UniversalId& universalId);

        protected slots:

            void closeRequest();
    };
}

#endif
