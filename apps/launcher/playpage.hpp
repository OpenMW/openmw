#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include <QWidget>

class QComboBox;
class QStringListModel;

class PlayPage : public QWidget
{
    Q_OBJECT

public:
    PlayPage(QWidget *parent = 0);

    QComboBox *mProfilesComboBox;
    QStringListModel *mProfilesModel;
};

#endif