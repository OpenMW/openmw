#include "scalableicon.hpp"

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QIODevice>
#include <QIconEngine>
#include <QPainter>
#include <QPalette>
#include <QtSvg/QSvgRenderer>

namespace Misc
{
    Q_GLOBAL_STATIC(QSet<ScalableIcon*>, ScalableIconInstances)

    ScalableIcon::ScalableIcon(const QByteArray& svgContent)
        : mTemplate(svgContent)
    {
        update();
        ScalableIconInstances->insert(this);
    }

    ScalableIcon::~ScalableIcon()
    {
        if (!ScalableIconInstances.isDestroyed())
        {
            ScalableIconInstances->remove(this);
        }
    }

    QIcon Misc::ScalableIcon::load(const QString& fileName)
    {
        if (fileName.isEmpty())
            return QIcon();

        QFile iconFile(fileName);
        if (!iconFile.open(QIODevice::ReadOnly))
        {
            qDebug() << "Failed to open icon file:" << fileName;
            return QIcon();
        }

        auto content = iconFile.readAll();
        if (!content.startsWith("<?xml"))
            return QIcon(fileName);

        return QIcon(new ScalableIcon(content));
    }

    void ScalableIcon::update()
    {
        constexpr const char* templateColor = "#4d4d4d";
        mContent = mTemplate;

        auto themeColor = QApplication::palette().text().color().name().toLower().toLatin1();
        mContent.replace(templateColor, themeColor);
    }

    void ScalableIcon::updateAllIcons()
    {
        for (auto engine : *ScalableIconInstances)
        {
            engine->update();
        }
    }

    void ScalableIcon::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
    {
        Q_UNUSED(mode);
        Q_UNUSED(state);

        QSvgRenderer renderer(mContent);
        renderer.render(painter, rect);
    }

    QIconEngine* ScalableIcon::clone() const
    {
        return new ScalableIcon(*this);
    }

    QPixmap ScalableIcon::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
    {
        QPixmap pix = QPixmap(size);
        pix.fill(Qt::transparent);

        QPainter painter(&pix);
        QRect r(QPoint(0.0, 0.0), size);
        this->paint(&painter, r, mode, state);

        if (mode != QIcon::Disabled)
            return pix;

        // For disabled icons use grayscale icons with 50% transparency
        QImage img = pix.toImage();

        for (int x = 0; x < img.width(); x++)
        {
            for (int y = 0; y < img.height(); y++)
            {
                QColor n = img.pixelColor(x, y);
                int gray = qGray(n.red(), n.green(), n.blue());
                img.setPixelColor(x, y, QColor(gray, gray, gray, n.alpha() / 2));
            }
        }

        return QPixmap::fromImage(img, Qt::NoFormatConversion);
    }
}
