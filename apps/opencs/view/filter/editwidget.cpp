
#include "editwidget.hpp"

#include <QAbstractItemModel>
#include <QString>

#include "../../model/world/data.hpp"

CSVFilter::EditWidget::EditWidget (CSMWorld::Data& data, QWidget *parent)
: QLineEdit (parent), mParser (data)
{
    mPalette = palette();
    connect (this, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));

    QAbstractItemModel *model = data.getTableModel (CSMWorld::UniversalId::Type_Filters);

    connect (model, SIGNAL (dataChanged (const QModelIndex &, const QModelIndex&)),
        this, SLOT (filterDataChanged (const QModelIndex &, const QModelIndex&)),
        Qt::QueuedConnection);
    connect (model, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (filterRowsRemoved (const QModelIndex&, int, int)),
        Qt::QueuedConnection);
    connect (model, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (filterRowsInserted (const QModelIndex&, int, int)),
        Qt::QueuedConnection);
}

void CSVFilter::EditWidget::textChanged (const QString& text)
{
    if (mParser.parse (text.toUtf8().constData()))
    {
        setPalette (mPalette);
        emit filterChanged (mParser.getFilter());
    }
    else
    {
        QPalette palette (mPalette);
        palette.setColor (QPalette::Text, Qt::red);
        setPalette (palette);

        /// \todo improve error reporting; mark only the faulty part
    }
}

void CSVFilter::EditWidget::filterDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    textChanged (text());
}

void CSVFilter::EditWidget::filterRowsRemoved (const QModelIndex& parent, int start, int end)
{
    textChanged (text());
}

void CSVFilter::EditWidget::filterRowsInserted (const QModelIndex& parent, int start, int end)
{
    textChanged (text());
}

void CSVFilter::EditWidget::createFilterRequest (std::vector< std::pair< std::string, std::vector< std::string > > >& filterSource)
{
    const unsigned count = filterSource.size();
    bool multipleElements = false;

    switch (count)
    {
        case 0: //empty
            return; //nothing to do here

        case 1: //only single
            multipleElements = false;
            break;

        default:
            multipleElements = true;
            break;
    }

    QString oldContent(text());
    bool replaceMode = oldContent.isEmpty() or !oldContent.contains(QRegExp("^!.*$", Qt::CaseInsensitive));
    bool orMode = true; //not orMode = andMode,

    std::string orAnd;
    if (orMode)
    {
        orAnd = "or";
    } else {
        orAnd = "and";
    }

    if (multipleElements) //TODO Currently only 'or' is handled, we should be able to handle 'and' as well and be able to drag records into the EditWidget already filled with the filter
    {
        std::stringstream ss;
        if (replaceMode)
        {
            ss<<'!'<<orAnd<<'(';

            for (unsigned i = 0; i < count; ++i)
            {
                ss<<generateFilter (filterSource[i]);

                if (i+1 != count)
                {
                    ss<<", ";
                }
            }

            ss<<')';
            clear();
            insert (QString::fromUtf8 (ss.str().c_str()));
        } else {
            //not handled (yet) TODO
        }
    } else {
        if (replaceMode)
        {
            clear();
            insert ('!' + QString::fromUtf8 (generateFilter(filterSource[0]).c_str()));
        } else {
            //not handled (yet) TODO
        }
    }
}

std::string CSVFilter::EditWidget::generateFilter (std::pair< std::string, std::vector< std::string > >& seekedString) const
{
    const unsigned columns = seekedString.second.size();

    bool multipleColumns = false;
    switch (columns)
    {
    case 0: //empty
        return ""; //no column to filter

    case 1: //one column to look for
        multipleColumns = false;
        break;

    default:
        multipleColumns = true;
        break;
    }

    std::stringstream ss;
    if (multipleColumns)
    {
        ss<<"or(";
        for (unsigned i = 0; i < columns; ++i)
        {
            ss<<"string("<<'"'<<seekedString.second[i]<<'"'<<','<<'"'<<seekedString.first<<'"'<<')';
            if (i+1 != columns)
                ss<<',';
        }
        ss<<')';
    } else {
        ss<<"string"<<'('<<'"'<<seekedString.second[0]<<"\","<<'"'<<seekedString.first<<"\")";
    }

    return ss.str();
}
