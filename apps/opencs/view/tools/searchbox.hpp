#ifndef CSV_TOOLS_SEARCHBOX_H
#define CSV_TOOLS_SEARCHBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>

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
            QStackedWidget mReplaceInput;
            QLineEdit mReplaceText;
            QLabel mReplacePlaceholder;

        private:

            void updateSearchButton();
            
        public:

            SearchBox (QWidget *parent = 0);

            void setSearchMode (bool enabled);

            CSMTools::Search getSearch() const;

            std::string getReplaceText() const;

        private slots:

            void modeSelected (int index);

            void textChanged (const QString& text);

            void startSearch (bool checked = true);

        signals:

            void startSearch (const CSMTools::Search& search);
    };
}

#endif
