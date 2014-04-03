
#include "scenetoolgrid.hpp"

#include <sstream>

#include <QPainter>
#include <QApplication>
#include <QFontMetrics>

#include "scenetoolbar.hpp"

CSVWorld::SceneToolGrid::SceneToolGrid (SceneToolbar *parent)
: SceneTool (parent), mIconSize (parent->getIconSize())
{
}

void CSVWorld::SceneToolGrid::showPanel (const QPoint& position)
{


}

void CSVWorld::SceneToolGrid::cellIndexChanged (const std::pair<int, int>& min,
    const std::pair<int, int>& max)
{
    /// \todo make font size configurable
    const int fontSize = 8;

    /// \todo replace with proper icon
    QPixmap image (mIconSize, mIconSize);
    image.fill (QColor (0, 0, 0, 0));

    {
        QPainter painter (&image);
        painter.setPen (Qt::black);
        QFont font (QApplication::font().family(), fontSize);
        painter.setFont (font);

        QFontMetrics metrics (font);

        if (min==max)
        {
            // single cell
            std::ostringstream stream;
            stream << min.first << ", " << min.second;

            QString text = QString::fromUtf8 (stream.str().c_str());

            painter.drawText (QPoint ((mIconSize-metrics.width (text))/2, mIconSize/2+fontSize/2),
                text);
        }
        else
        {
            // range
            {
                std::ostringstream stream;
                stream << min.first << ", " << min.second;
                painter.drawText (QPoint (0, mIconSize),
                    QString::fromUtf8 (stream.str().c_str()));
            }

            {
                std::ostringstream stream;
                stream << max.first << ", " << max.second;

                QString text = QString::fromUtf8 (stream.str().c_str());

                painter.drawText (QPoint (mIconSize-metrics.width (text), fontSize), text);
            }
        }
    }

    QIcon icon (image);
    setIcon (icon);
}