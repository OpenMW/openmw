#ifndef PROFILESCOMBOBOX_HPP
#define PROFILESCOMBOBOX_HPP

#include <QComboBox>
#include <QStylePainter>
class QString;
class QRegExpValidator;

namespace ContentSelectorView
{
    class ProfilesComboBox : public QComboBox
    {
        Q_OBJECT
    public:
        explicit ProfilesComboBox(QWidget *parent = 0);
        void setEditEnabled(bool editable);
        void setPlaceholderText(const QString &text);
      //  void indexChanged(int index);

    signals:
        void signalProfileTextChanged(const QString &item);
        void signalProfileChanged(const QString &previous, const QString &current);
        void signalProfileChanged(int index);
        void profileRenamed(const QString &oldName, const QString &newName);

    private slots:

        void slotEditingFinished();
        void slotIndexChangedByUser(int index);
        void slotTextChanged(const QString &text);

    private:
        QString mOldProfile;
        QString mPlaceholderText;
        QRegExpValidator *mValidator;

    protected:
        void paintEvent(QPaintEvent *);
    };
}

#endif // PROFILESCOMBOBOX_HPP
