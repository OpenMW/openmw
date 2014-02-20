
#include "editwidget.hpp"

#include <QAbstractItemModel>

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
    for (unsigned i = 0; i < filterSource.size(); ++i) //test
    {
        std::cout<<filterSource[i].first<<std::endl;
        std::cout<<"Columns:\n";
        for (unsigned j = 0; j < filterSource[i].second.size(); ++j)
        {
            std::cout<<filterSource[i].second[j]<<std::endl;
        }
        std::cout<<"\n";
    }
}