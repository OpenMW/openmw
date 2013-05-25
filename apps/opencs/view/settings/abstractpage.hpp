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

    class AbstractPage: public QWidget
    {

    protected:

        AbstractBlockList mAbstractBlocks;

    public:

        AbstractPage(QWidget *parent = 0);
        AbstractPage (const QString &pageName, QWidget* parent = 0);

        ~AbstractPage();

        virtual void setupUi()=0;

        virtual void initializeWidgets (const CSMSettings::SettingMap &settings) = 0;

        CSMSettings::SettingList *getSettings();

        void setObjectName();

    protected:

        template <typename S, typename T>
        AbstractBlock *buildBlock (T &def)
        {
            S *block = new S (this);
            int ret = block->build (def);

            if (ret < 0)
                return 0;

            QWidget::layout()->addWidget (block->getGroupBox());

            return block;
        }


    };
}

#endif // ABSTRACTPAGE_HPP
