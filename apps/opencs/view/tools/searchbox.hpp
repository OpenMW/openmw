#ifndef CSV_TOOLS_SEARCHBOX_H
#define CSV_TOOLS_SEARCHBOX_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

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
        QCheckBox mCaseSensitive;
        QPushButton mSearch;
        QGridLayout* mLayout;
        bool mLocked;
        QComboBox mMode;
        bool mSearchEnabled;
        QStackedWidget mReplaceInput;
        QLineEdit mReplaceText;
        QLabel mReplacePlaceholder;
        QPushButton mReplace;

    private:
        void updateSearchButtons();

        bool canReplace();
        bool canReplaceRegex();
        bool canSearch();
        bool canSearchRegex();

        bool hasValidSearchText();
        bool hasValidSearchRegex();

    public:
        SearchBox(QWidget* parent = nullptr);

        void setSearchMode(bool enabled);

        CSMTools::Search getSearch() const;

        std::string getReplaceText() const;

        void setEditLock(bool locked);

        void focus();

    private slots:

        void modeSelected(int index);

        void textChanged(const QString& text);

        void startSearch(bool checked = true);

        void replaceAll(bool checked);

    signals:

        void startSearch(const CSMTools::Search& search);

        void replaceAll();
    };
}

#endif
