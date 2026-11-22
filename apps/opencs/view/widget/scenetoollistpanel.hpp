#ifndef OPENCS_VIEW_WIDGET_SCENETOOLLISTPANEL_HPP
#define OPENCS_VIEW_WIDGET_SCENETOOLLISTPANEL_HPP

#include <vector>

#include <QFrame>
#include <QStringList>

class QModelIndex;
class QPoint;
class QTableWidget;
class QWidget;

namespace CSVWidget
{
    /// \brief Popup list that a scene tool shows when its button is right-clicked
    ///
    /// Each row consists of its text cells followed by a remove button. The panel only displays
    /// the rows; the tool owns the data and calls setRows after changing it.
    class SceneToolListPanel : public QFrame
    {
        Q_OBJECT

    public:
        SceneToolListPanel(int textColumnCount, QWidget* parent);

        /// \note The panel closes when it is left without rows.
        void setRows(const std::vector<QStringList>& rows);

        /// \note A panel without rows stays closed.
        void showAt(const QPoint& position);

    signals:

        /// A text cell of \a row was clicked. The panel closes before this is emitted.
        void selectRequest(int row);

        /// The remove button of \a row was clicked. The panel stays open.
        void removeRequest(int row);

    private slots:

        void clicked(const QModelIndex& index);

    private:
        QTableWidget* mTable;
    };
}

#endif
