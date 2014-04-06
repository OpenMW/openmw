#include "listview.hpp"
#include "../../model/settings/setting.hpp"
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
    QWidget *widget = buildWidget();
    buildView (widget);
    buildModel (widget);
}

void CSVSettings::ListView::buildModel(QWidget *widget)
{
    if (mComboBox)
    {
        mComboBox->setModel (dataModel());
        mComboBox->view()->setSelectionModel (selectionModel());
    }
    else
    {
        mAbstractItemView->setModel (dataModel());
        mAbstractItemView->setSelectionModel (selectionModel());
    }

    connect (mComboBox, SIGNAL(currentIndexChanged(int)),
             this, SLOT (slotIndexChanged(int)));
}

void CSVSettings::ListView::slotIndexChanged (int idx) const
{
    qDebug() << objectName() << "::ListView::slotIndexChanged() = " << idx;
}

void CSVSettings::ListView::buildView (QWidget *widget)
{
    if (setting()->widgetWidth() > 0)
    {
        QString widthToken;
        widthToken.fill('P', setting()->widgetWidth());
        QFontMetrics fm (QApplication::font());
        widget->setFixedWidth (fm.width (widthToken));
    }

    viewFrame()->addWidget (widget, setting()->viewRow(),
                            setting()->viewColumn());
}

QWidget *CSVSettings::ListView::buildWidget()
{
    QWidget *widget = 0;

    if (setting()->isMultiLine())
    {
        mAbstractItemView = new QListView (this);
        widget = mAbstractItemView;
    }
    else
    {
        mComboBox = new QComboBox (this);
        widget = mComboBox;
    }
    return widget;
}

void CSVSettings::ListView::showEvent ( QShowEvent * event )
{
    View::showEvent (event);

    if (mComboBox)
    {
        mComboBox->setModel (dataModel());
        mComboBox->view()->setSelectionModel (selectionModel());

        if (!selectionModel()->selection().isEmpty())
        {
             mComboBox->setCurrentIndex (selectionModel()->
                                                selectedIndexes().at(0).row());

        }
       else
            mComboBox->setCurrentIndex (-1);

        mComboBox->setModelColumn (0);
    }
    else if (mAbstractItemView)
    {
        mAbstractItemView->setModel (dataModel());
        mAbstractItemView->setSelectionModel (selectionModel());
    }
}

void CSVSettings::ListView::updateView() const
{
    QStringList values = selectedValues();

    qDebug() << "list view update";
    int idx = -1;

    if (mComboBox)
    {
        if (values.size() > 0)
            idx =  (mComboBox->findText(values.at(0)));

        mComboBox->setCurrentIndex (idx);
    }
    View::updateView();
}

CSVSettings::ListView *CSVSettings::ListViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new ListView(setting, parent);
}
