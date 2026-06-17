#ifndef SCALABLEICON_HPP
#define SCALABLEICON_HPP

#include <QIconEngine>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace Misc
{
    class ScalableIcon : public QIconEngine
    {
    public:
        QIconEngine* clone() const override;

        QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

        void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

        static void updateAllIcons();

        virtual ~ScalableIcon();

        static QIcon load(const QString& fileName);

    private:
        explicit ScalableIcon(const QByteArray& svgContent);

        void update();

        QByteArray mTemplate;
        QByteArray mContent;
    };
}

#endif // SCALABLEICON_HPP
