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

<<<<<<< Updated upstream
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
=======
    public:
        PlayPage(QWidget *parent = 0);
        void setProfilesModel(QAbstractItemModel *model);

    signals:
        void signalProfileChanged(int index);
        void playButtonClicked();

    public slots:
        void setProfilesIndex(int index);

    private slots:
        void slotPlayClicked();
>>>>>>> Stashed changes



    };
}
#endif
