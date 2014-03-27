#include "listview.hpp"
#include "../../model/settings/setting.hpp"
#include "../../model/settings/selector.hpp"
#include "settingbox.hpp"

#include <QListView>
#include <QComboBox>
#include <QApplication>
#include <QDebug>
#include <QStringListModel>

CSVSettings::ListView::ListView(CSMSettings::Setting *setting,
                                Page *parent)
    : mComboBox (0), mAbstractItemView (0), View(setting, parent)
{
    QWidget *widget = 0;

    if (setting->isMultiLine())
    {
        mAbstractItemView = new QListView (this);
        widget = mAbstractItemView;
    }
    else
    {
        mComboBox = new QComboBox (this);
        widget = mComboBox;
    }

    if (setting->widgetWidth() > 0)
    {
        QString widthToken;
        widthToken.fill('P', setting->widgetWidth());
        QFontMetrics fm (QApplication::font());
        widget->setFixedWidth (fm.width (widthToken));
    }

    viewFrame()->addWidget (widget, setting->viewRow(),
                            setting->viewColumn());
}

void CSVSettings::ListView::showEvent ( QShowEvent * event )
{
    if (!selector())
        View::showEvent (event);

    if (mComboBox)
    {
        mComboBox->setModel (selector()->model());
        mComboBox->view()->setSelectionModel (selector()->selectionModel());

         if (!selector()->selectedRows().isEmpty())
        {
            mComboBox->setCurrentIndex (selector()->selectedRows().back());
        }
       else
            mComboBox->setCurrentIndex (-1);

        mComboBox->setModelColumn (0);
    }
    else if (mAbstractItemView)
    {
        mAbstractItemView->setModel (selector()->model());
        mAbstractItemView->setSelectionModel (selector()->selectionModel());
    }
}

void CSVSettings::ListView::slotUpdateView (QStringList list)
{
    qDebug() << "list view update";
    int idx = -1;

    if (mComboBox)
    {
        if (list.size() > 0)
            idx =  (mComboBox->findText(list.at(0)));

        mComboBox->setCurrentIndex (idx);
    }
}

CSVSettings::ListView *CSVSettings::ListViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new ListView(setting, parent);
}
