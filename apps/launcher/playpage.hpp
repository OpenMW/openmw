#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include <QWidget>

#include "ui_playpage.h"

class QComboBox;
class QPushButton;
class QAbstractItemModel;

namespace Launcher
{
    class PlayPage : public QWidget, private Ui::PlayPage
    {
        Q_OBJECT

    public:
        PlayPage(QWidget *parent = 0);
        void setProfilesModel(QAbstractItemModel *model);

    signals:
        void signalProfileChanged(int index);
        void playButtonClicked();
        void csButtonClicked();

    public slots:
        void setProfilesIndex(int index);

    private slots:
        void slotPlayClicked();
        void slotCSClicked();

    };
}
#endif
