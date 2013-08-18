
#include "editwidget.hpp"

CSVFilter::EditWidget::EditWidget (QWidget *parent)
: QLineEdit (parent)
{
    connect (this, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));
}

void CSVFilter::EditWidget::textChanged (const QString& text)
{
    mParser.parse (text.toUtf8().constData());

    if (mParser.getState()==CSMFilter::Parser::State_End)
        emit filterChanged (mParser.getFilter(), "");
    else
    {
        /// \todo error handling
    }
}