#ifndef CSMSETTINGS_DECLARATIONITEM_HPP
#define CSMSETTINGS_DECLARATIONITEM_HPP

#include <QStandardItem>
#include "../../view/settings/support.hpp"

namespace CSMSettings
{
    class ISettingDeclaration
    {
        virtual bool setMultiValue (bool isMultiValue) = 0;
        virtual bool setMultiValue (bool isMultiValue) = 0;
        virtual bool setHorizontal (bool isHorizontal) = 0;
        virtual bool setWidgetWidth (int width) = 0;
        virtual bool setViewRow (int row) = 0;
        virtual bool setViewColumn (int column) = 0;
        virtual bool setProperty (SettingProperty prop, int value) = 0;
        virtual bool setProperty (SettingProperty prop, bool value) = 0;
        virtual bool setProperty (SettingProperty prop, const QString &value)=0;
        virtual void setValueList (const QStringList list) = 0;

    };

    class DeclarationItem : public QStandardItem, public ISettingDeclaration
    {
    public:
        explicit DeclarationItem();

        bool setMultiValue (bool isMultiValue)
                    { return setProperty (Setting_IsMultiValue, isMultiValue); }

        bool setMultiLine (bool isMultiLine)
                    { return setProperty (Setting_IsMultiLine, isMultiLine); }

        bool setHorizontal (bool isHorizontal)
                    { return setProperty (Setting_IsHorizontal, isHorizontal); }

        bool setWidgetWidth (int width)
                    { return setProperty (Setting_WidgetWidth, width); }

        bool setViewRow (int row)
                    { return setProperty (Setting_ViewRow, row); }

        bool setViewColumn (int column)
                    { return setProperty (Setting_ViewColumn, column); }

        bool setProperty (SettingProperty prop, int value);
        bool setProperty (SettingProperty prop, bool value);
        bool setProperty (SettingProperty prop, const QString &value);
        void setValueList (const QStringList list);
    };
}
#endif // CSMSETTINGS_DECLARATIONITEM_HPP
