#ifndef CSV_TOOLS_SEARCHBOX_H
#define CSV_TOOLS_SEARCHBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>

class QGridLayout;

namespace CSMTools
{
    class Search;
}

namespace CSVTools
{
    class SearchBox : public QWidget
    {
            Q_OBJECT

            QStackedWidget mInput;
            QLineEdit mText;
            QComboBox mRecordState;
            QPushButton mSearch;
            QGridLayout *mLayout;
            QComboBox mMode;
            bool mSearchEnabled;

        private:

            void updateSearchButton();
            
        public:

            SearchBox (QWidget *parent = 0);

            void setSearchMode (bool enabled);

            CSMTools::Search getSearch() const;

        private slots:

            void modeSelected (int index);

            void textChanged (const QString& text);

            void startSearch (bool checked);

        signals:

            void startSearch (const CSMTools::Search& search);
    };
}

#endif
