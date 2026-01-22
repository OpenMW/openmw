#include "searchbox.hpp"

#include <stdexcept>

#include <QGridLayout>

#include "../../model/world/columns.hpp"

#include "../../model/tools/search.hpp"

void CSVTools::SearchBox::updateSearchButton()
{
    if (!mSearchEnabled)
        mSearch.setEnabled(false);
    else
    {
        switch (mMode.currentIndex())
        {
            case 0:
            case 2:

                mSearch.setEnabled(!mText.text().isEmpty());
                break;

            case 1:
            case 3:
            {
                if (mText.text().isEmpty())
                    mSearch.setEnabled(false);
                else
                    mSearch.setEnabled(QRegularExpression(mText.text()).isValid());
                break;
            }

            case 4:

                mSearch.setEnabled(true);
                break;
        }
    }
}

CSVTools::SearchBox::SearchBox(QWidget* parent)
    : QWidget(parent)
    , mSearch(tr("Search"))
    , mSearchEnabled(false)
    , mReplace(tr("Replace All"))
{
    mLayout = new QGridLayout(this);

    // search panel
    std::vector<std::pair<int, std::string>> states
        = CSMWorld::Columns::getEnums(CSMWorld::Columns::ColumnId_Modification);
    states.resize(states.size() - 1); // ignore erased state

    for (std::vector<std::pair<int, std::string>>::const_iterator iter(states.begin()); iter != states.end(); ++iter)
        mRecordState.addItem(QString::fromUtf8(iter->second.c_str()));

    mMode.addItem(tr("Text"));
    mMode.addItem(tr("Text (RegEx)"));
    mMode.addItem(tr("ID"));
    mMode.addItem(tr("ID (RegEx)"));
    mMode.addItem(tr("Record State"));
    connect(&mMode, qOverload<int>(&QComboBox::activated), this, &SearchBox::modeSelected);
    mLayout->addWidget(&mMode, 0, 0);

    connect(&mText, &QLineEdit::textChanged, this, &SearchBox::textChanged);
    connect(&mText, &QLineEdit::returnPressed, this, [this]() { this->startSearch(false); });
    mInput.insertWidget(0, &mText);

    mInput.insertWidget(1, &mRecordState);
    mLayout->addWidget(&mInput, 0, 1);

    mCaseSensitive.setText(tr("Case"));
    mLayout->addWidget(&mCaseSensitive, 0, 2);

    connect(&mSearch, &QPushButton::clicked, this, qOverload<bool>(&SearchBox::startSearch));
    mLayout->addWidget(&mSearch, 0, 3);

    // replace panel
    mReplaceInput.insertWidget(0, &mReplaceText);
    mReplaceInput.insertWidget(1, &mReplacePlaceholder);

    mLayout->addWidget(&mReplaceInput, 1, 1);

    mLayout->addWidget(&mReplace, 1, 3);

    // layout adjustments
    mLayout->setColumnMinimumWidth(2, 50);
    mLayout->setColumnStretch(1, 1);

    mLayout->setContentsMargins(0, 0, 0, 0);

    connect(&mReplace, &QPushButton::clicked, this, qOverload<bool>(&SearchBox::replaceAll));

    // update
    modeSelected(0);

    updateSearchButton();
}

void CSVTools::SearchBox::setSearchMode(bool enabled)
{
    mSearchEnabled = enabled;
    updateSearchButton();
}

CSMTools::Search CSVTools::SearchBox::getSearch() const
{
    CSMTools::Search::Type type = static_cast<CSMTools::Search::Type>(mMode.currentIndex());
    bool caseSensitive = mCaseSensitive.isChecked();

    switch (type)
    {
        case CSMTools::Search::Type_Text:
        case CSMTools::Search::Type_Id:

            return CSMTools::Search(type, caseSensitive, std::string(mText.text().toUtf8().data()));

        case CSMTools::Search::Type_TextRegEx:
        case CSMTools::Search::Type_IdRegEx:

            return CSMTools::Search(type, caseSensitive,
                QRegularExpression(mText.text().toUtf8().data(), QRegularExpression::CaseInsensitiveOption));

        case CSMTools::Search::Type_RecordState:

            return CSMTools::Search(type, caseSensitive, mRecordState.currentIndex());

        case CSMTools::Search::Type_None:

            break;
    }

    throw std::logic_error("invalid search mode index");
}

std::string CSVTools::SearchBox::getReplaceText() const
{
    CSMTools::Search::Type type = static_cast<CSMTools::Search::Type>(mMode.currentIndex());

    switch (type)
    {
        case CSMTools::Search::Type_Text:
        case CSMTools::Search::Type_TextRegEx:
        case CSMTools::Search::Type_Id:
        case CSMTools::Search::Type_IdRegEx:

            return mReplaceText.text().toUtf8().data();

        default:

            throw std::logic_error("Invalid search mode for replace");
    }
}

void CSVTools::SearchBox::setEditLock(bool locked)
{
    mReplace.setEnabled(!locked);
}

void CSVTools::SearchBox::focus()
{
    mInput.currentWidget()->setFocus();
}

void CSVTools::SearchBox::modeSelected(int index)
{
    switch (index)
    {
        case CSMTools::Search::Type_Text:
        case CSMTools::Search::Type_TextRegEx:
        case CSMTools::Search::Type_Id:
        case CSMTools::Search::Type_IdRegEx:

            mInput.setCurrentIndex(0);
            mReplaceInput.setCurrentIndex(0);
            break;

        case CSMTools::Search::Type_RecordState:
            mInput.setCurrentIndex(1);
            mReplaceInput.setCurrentIndex(1);
            break;
    }

    mInput.currentWidget()->setFocus();

    updateSearchButton();
}

void CSVTools::SearchBox::textChanged(const QString& text)
{
    updateSearchButton();
}

void CSVTools::SearchBox::startSearch(bool checked)
{
    if (mSearch.isEnabled())
        emit startSearch(getSearch());
}

void CSVTools::SearchBox::replaceAll(bool checked)
{
    emit replaceAll();
}
