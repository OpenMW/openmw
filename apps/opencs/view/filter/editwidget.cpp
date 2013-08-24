
#include "editwidget.hpp"

CSVFilter::EditWidget::EditWidget (const CSMWorld::Data& data, QWidget *parent)
: QLineEdit (parent), mParser (data)
{
    mPalette = palette();
    connect (this, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));
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