#ifndef DATADISPLAYDELEGATE_HPP
#define DATADISPLAYDELEGATE_HPP

#include <QTextOption>
#include "enumdelegate.hpp"

namespace CSVWorld
{


    class DataDisplayDelegate : public EnumDelegate
    {
    public:

        typedef std::vector < std::pair < int, QIcon > > IconList;
        typedef std::vector<std::pair<int, QString> > ValueList;

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

        std::vector <std::pair <int, QPixmap> > mPixmaps;
        QTextOption mTextAlignment;
        QSize mIconSize;
        int mIconLeftOffset;
        int mTextLeftOffset;

        QString mSettingKey;

    public:
        explicit DataDisplayDelegate (const ValueList & values,
                                      const IconList & icons,
                                      CSMDoc::Document& document,
                                      const QString &pageName,
                                      const QString &settingName,
                                      QObject *parent);

        ~DataDisplayDelegate();

        virtual void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

        /// pass a QSize defining height / width of icon. Default is QSize (16,16).
        void setIconSize (const QSize& icon);

        /// offset the horizontal position of the icon from the left edge of the cell.  Default is 3 pixels.
        void setIconLeftOffset (int offset);

        /// offset the horizontal position of the text from the right edge of the icon.  Default is 8 pixels.
        void setTextLeftOffset (int offset);

        ///update the display mode for the delegate
        void updateUserSetting (const QString &name, const QStringList &list);

    private:

        /// update the display mode based on a passed string
        void updateDisplayMode (const QString &);

        /// custom paint function for painting the icon.  Mode_IconAndText and Mode_Icon only.
        void paintIcon (QPainter *painter, const QStyleOptionViewItem &option, int i) const;

        /// rebuild the list of pixmaps from the provided icons (called when icon size is changed)
        void buildPixmaps();

    };

    class DataDisplayDelegateFactory : public EnumDelegateFactory
    {
    protected:

        DataDisplayDelegate::IconList mIcons;

    public:

        virtual CommandDelegate *makeDelegate (CSMDoc::Document& document, QObject *parent) const;
        ///< The ownership of the returned CommandDelegate is transferred to the caller.

    protected:

       void add (int enumValue,const  QString enumName, const QString iconFilename);

    };

}

#endif // DATADISPLAYDELEGATE_HPP
