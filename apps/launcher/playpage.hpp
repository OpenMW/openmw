#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include <QWidget>

#include "ui_playpage.h"

class QComboBox;
class QPushButton;
class QAbstractItemModel;

class PlayPage : public QWidget, private Ui::PlayPage
{
    Q_OBJECT

public:
    PlayPage(QWidget *parent = 0);
    void setProfilesComboBoxModel(QAbstractItemModel *model);

signals:
    void profileChanged(int index);
    void playButtonClicked();

public slots:
    void setProfilesComboBoxIndex(int index);

private slots:
    void slotCurrentIndexChanged(int index);
    void slotPlayClicked();



};

#endif
