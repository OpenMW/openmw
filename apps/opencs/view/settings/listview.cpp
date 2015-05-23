#include "listview.hpp"
#include "../../model/settings/setting.hpp"

#include <QListView>
#include <QComboBox>
#include <QStringListModel>

CSVSettings::ListView::ListView(CSMSettings::Setting *setting,
                                Page *parent)
    : View(setting, parent), mAbstractItemView (0), mComboBox (0)
{
    QWidget *widget =
                    buildWidget(setting->isMultiLine(), setting->widgetWidth());

    addWidget (widget, setting->viewRow(), setting->viewColumn());

    if (mComboBox)
        buildComboBoxModel();

    else if (mAbstractItemView)
        buildAbstractItemViewModel();
}

void CSVSettings::ListView::buildComboBoxModel()
{
    mComboBox->setModel (dataModel());
    mComboBox->setModelColumn (0);
    mComboBox->view()->setSelectionModel (selectionModel());

    int curIdx = -1;

    if (!selectionModel()->selection().isEmpty())
        curIdx = selectionModel()->selectedIndexes().at(0).row();

     mComboBox->setCurrentIndex (curIdx);

     connect (mComboBox, SIGNAL(currentIndexChanged(int)),
              this, SLOT(emitItemViewUpdate(int)));
}

void CSVSettings::ListView::buildAbstractItemViewModel()
{
    mAbstractItemView->setModel (dataModel());
    mAbstractItemView->setSelectionModel (selectionModel());

    //connection needs to go here for list view update to signal to
    //the outside
}

void CSVSettings::ListView::emitItemViewUpdate (int idx)
{
    updateView();
}

QWidget *CSVSettings::ListView::buildWidget(bool isMultiLine, int width)
{
    QWidget *widget = 0;

    if (isMultiLine)
    {
        mAbstractItemView = new QListView (this);
        widget = mAbstractItemView;

        if (width > 0)
            widget->setFixedWidth (widgetWidth (width));
    }
    else
    {
        mComboBox = new QComboBox (this);
        widget = mComboBox;

        if (width > 0)
            mComboBox->setMinimumContentsLength (width);
    }

    return widget;
}

void CSVSettings::ListView::showEvent ( QShowEvent * event )
{
    View::showEvent (event);
}

void CSVSettings::ListView::updateView (bool signalUpdate) const
{
    QStringList values = selectedValues();

    if (mComboBox)
    {
        int idx = -1;

        if (values.size() > 0)
            idx =  (mComboBox->findText(values.at(0)));

        mComboBox->setCurrentIndex (idx);
    }

    View::updateView (signalUpdate);
}

CSVSettings::ListView *CSVSettings::ListViewFactory::createView
                                        (CSMSettings::Setting *setting,
                                         Page *parent)
{
    return new ListView(setting, parent);
}
