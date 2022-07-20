#ifndef PLAYPAGE_H
#define PLAYPAGE_H

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
        PlayPage(QWidget *parent = nullptr);
        void setProfilesModel(QAbstractItemModel *model);

    signals:
        void signalProfileChanged(int index);
        void playButtonClicked();

    public slots:
        void setProfilesIndex(int index);

    private slots:
        void slotPlayClicked();



    };
}
#endif
