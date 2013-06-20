#ifndef ABSTRACTPAGE_HPP
#define ABSTRACTPAGE_HPP

#include <QWidget>
#include <QList>
#include <QLayout>

#include "abstractblock.hpp"

class SettingMap;
class SettingList;

namespace CSVSettings {

    typedef QList<AbstractBlock *> AbstractBlockList;

    /// Abstract base class for all setting pages in the dialog

    /// \todo Scripted implementation of settings should eliminate the need
    /// \todo derive page classes.
    /// \todo AbstractPage should be replaced with a general page construction class.
    class AbstractPage: public QWidget
    {

    protected:

        AbstractBlockList mAbstractBlocks;

    public:

        AbstractPage(QWidget *parent = 0);
        AbstractPage (const QString &pageName, QWidget* parent = 0);

        ~AbstractPage();

        virtual void setupUi() = 0;

        /// triggers widgiet initialization at the page level.  All widgets updated to
        /// current setting values
        virtual void initializeWidgets (const CSMSettings::SettingMap &settings) = 0;

        /// retrieve the list of settings local to the page.
        CSMSettings::SettingList *getSettings();

        void setObjectName();

    protected:

        /// Create a block for the page.
        /// Block is constructed using passed definition struct
        /// Page level-layout is created and assigned
        template <typename S, typename T>
        AbstractBlock *buildBlock (T *def)
        {
            S *block = new S (this);
            int ret = block->build (def);

            if (ret < 0)
                return 0;

            QGroupBox *box = block->getGroupBox();
            QWidget::layout()->addWidget (box);

            return block;
        }

    };
}

#endif // ABSTRACTPAGE_HPP
