#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include <QWidget>

class QComboBox;
class QPushButton;

class PlayPage : public QWidget
{
    Q_OBJECT

public:
    PlayPage(QWidget *parent = 0);

    QComboBox *mProfilesComboBox;
    QPushButton *mPlayButton;

};

#endif