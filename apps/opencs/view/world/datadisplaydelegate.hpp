#ifndef DATADISPLAYDELEGATE_HPP
#define DATADISPLAYDELEGATE_HPP

#include "enumdelegate.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

class QModelIndex;
class QObject;
class QPainter;
class QPixmap;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CommandDispatcher;
}

namespace CSMPrefs
{
    class Setting;
}

namespace CSVWorld
{
    class CommandDelegate;

    struct Icon
    {
        int mValue;
        QIcon mIcon;
        QString mName;
    };

    class DataDisplayDelegate : public EnumDelegate
    {
        Q_OBJECT
    public:
        typedef std::vector<Icon> IconList;
        typedef std::vector<std::pair<int, QString>> ValueList;

    protected:
        enum DisplayMode
        {
            Mode_TextOnly,
            Mode_IconOnly,
            Mode_IconAndText
        };

        DisplayMode mDisplayMode;
        IconList mIcons;

    private:
        std::vector<std::pair<int, QPixmap>> mPixmaps;
        QSize mIconSize;
        int mHorizontalMargin;
        int mTextLeftOffset;
        QColor mPixmapsColor;
        qreal mUiScale;

        std::string mSettingKey;

    public:
        DataDisplayDelegate(const ValueList& values, const IconList& icons, CSMWorld::CommandDispatcher* dispatcher,
            CSMDoc::Document& document, const std::string& pageName, const std::string& settingName, QObject* parent);

        ~DataDisplayDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        /// pass a QSize defining height / width of icon. Default is QSize (16,16).
        void setIconSize(const QSize& icon);

        /// offset the horizontal position of the text from the right edge of the icon.  Default is 8 pixels.
        void setTextLeftOffset(int offset);

        bool eventFilter(QObject* target, QEvent* event) override;

    private:
        /// update the display mode based on a passed string
        void updateDisplayMode(const std::string&);

        /// custom paint function for painting the icon.  Mode_IconAndText and Mode_Icon only.
        void paintIcon(QPainter* painter, const QStyleOptionViewItem& option, int i) const;

        /// rebuild the list of pixmaps from the provided icons (called when icon size is changed)
        void buildPixmaps();

        void settingChanged(const CSMPrefs::Setting* setting) override;
    };

    class DataDisplayDelegateFactory : public EnumDelegateFactory
    {
    protected:
        DataDisplayDelegate::IconList mIcons;

    public:
        CommandDelegate* makeDelegate(
            CSMWorld::CommandDispatcher* dispatcher, CSMDoc::Document& document, QObject* parent) const override;
        ///< The ownership of the returned CommandDelegate is transferred to the caller.

    protected:
        void add(int enumValue, const QString& enumName, const QString& iconFilename);
    };

}

#endif // DATADISPLAYDELEGATE_HPP
